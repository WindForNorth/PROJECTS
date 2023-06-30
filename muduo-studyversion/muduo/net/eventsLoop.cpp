#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <sys/eventfd.h>
#include <signal.h>
#include "eventsLoop.h"
#include "Poller.h"
#include "Channel.h"

__thread eventsLoop * t_loopInThisThread = nullptr;
const int eventsLoop::ktimeOutMs = 10000;

// 忽略SIGPIPE信号
class ignoreSIGPIPE
{
public:
    ignoreSIGPIPE()
    {
        signal(SIGPIPE, SIG_IGN);
    }
};
ignoreSIGPIPE ignoreObj; // 使用全局对象

eventsLoop::eventsLoop(bool ifUsePoller) : threadID_(currentThreadID()),
                                           looping_(false),
                                           quit_(false),
                                           poller_(Poller::defaultPoller(ifUsePoller, this)),
                                           callingPendingFunctors_(false),
                                           wakeupfd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)), // 0为初始值
                                           wakeupchannel_(new Channel(this, wakeupfd_)),
                                           timersque_(new TimerQueue(this))
{
    std::cout << "wakeupfd_: " << wakeupfd_ << std::endl;
    if (t_loopInThisThread)
    {
        //  此时应该退出
        abort();
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupchannel_->setReadCallback(std::bind(&eventsLoop::handleRead, this)); 
    wakeupchannel_->enableReading();                                           // updateChannel使得wakeupfd生效
}

eventsLoop *eventsLoop::getCurrentLoopThread() { return t_loopInThisThread; }

void eventsLoop::abortNotInLoopThread()
{
    std::cout << "cunrrent threadid: " << currentThreadID() << std::endl;
    std::cout << "create threadid: " << threadID_ << std::endl;
    abort();
}

void eventsLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    while (!quit_)
    {
        activeChannels_.clear();
        Timestamp receiveTime = poller_->poll(ktimeOutMs, &activeChannels_);
        for (auto it : activeChannels_)
        {
            std::cout << "eventsfd: " << it->fd() << "\n";
            it->handleEvent(receiveTime); // 处理事件
        }
        doPendingFunctors();
    }
    looping_ = false;
}

eventsLoop::~eventsLoop()
{
    assertInLoopThread();
    close(wakeupfd_);
    t_loopInThisThread = nullptr;
}

void eventsLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannels(channel);
}

TimerID eventsLoop::runAt(Timestamp stamp, const TimerCallBackType & cb)
{
    return timersque_->addTimer(cb, stamp, 0.0);
}

TimerID eventsLoop::runAfter(double delay, const TimerCallBackType & cb)
{
    Timestamp stamp(addTime(Timestamp::now(), delay));
    // 可以把addTime理解为在现在时间的基础上增加时间，然后返回一个增加了时间的时间戳
    return runAt(stamp, cb);
}

TimerID eventsLoop::runEvery(double interval, const TimerCallBackType & cb)
{
    Timestamp stamp(addTime(Timestamp::now(), interval));
    return timersque_->addTimer(cb, stamp, interval); // 通过指定addTimer的第三个参数interval（大于0）指定周期定时器
}

void eventsLoop::runInLoop(const std::function<void()> &cb)
{
    if (isInLoopThread())
    {
        // IO线程内则直接调用回调
        cb();
    }
    else
    {
        // 否则就是将其加入其创建的IO线程，保证即使其他线程调用到这个函数，也是安全的，还不需要锁
        queueInLoop(cb);
    }
}

void eventsLoop::queueInLoop(const std::function<void()> &cb)
{
    // 这块可能被多个线程使用
    {
        mutexLockGuard lock(mutex_); // 上锁，对象析构时会自动解锁，于是需要这么一个独特的局部环境
        pendingFunctorsList_.push_back(cb);
    }
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        // 当其他线程调用这个函数将wakeupfd_上的可读事件回调放入队列时，唤醒主IO线程及时响应用户超时事件
        // 至于为什么当callingPendingFunctors_为true的时候也需要唤醒，是因为其他线程也可能调用queueInLoop，即使只有一个线程，也可能因为用户
        // 的某些回调函数调用queueInLoop，于是
        // 产生新的用户回调，此时需要往唤醒主线程的fd中写东西，保证目前主线程退出之后能立马从poll中返回处理新的回调，不然就可能导致响应不及时
        if (!isInLoopThread())
            std::cout << "not in IO thread\n";
        if (callingPendingFunctors_)
            std::cout << "calling Functors\n";
        wakeup();
    }
}

void eventsLoop::doPendingFunctors()
{
    std::vector<std::function<void()>> functors;
    callingPendingFunctors_ = true;
    {
        // 意思就是说其他线程是可能调用超时回调的
        mutexLockGuard lock(mutex_);
        // 交换到局部变量,一方面减少临界区的代码长度，减少其他线程的阻塞时间，另一方面防止因为functor再次调用queueInLoop造成死锁
        functors.swap(pendingFunctorsList_);
    }
    for (auto it : functors)
    {
        it();
    }
    callingPendingFunctors_ = false;
}

void eventsLoop::wakeup()
{
    uint64_t ringer = 1;
    ssize_t n = write(wakeupfd_, &ringer, sizeof ringer);
    if (n != sizeof ringer)
    {
        std::cout << "wakeup error\n";
    }
}

void eventsLoop::handleRead()
{
    uint64_t msg = 1;
    ssize_t n = read(wakeupfd_, &msg, sizeof msg);
    if (n != sizeof msg)
    {
        std::cout << "read wakeupfd error\n";
    }
    std::cout << "wakeupCount is: " << msg << "\n";
}

void eventsLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}