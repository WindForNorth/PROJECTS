#include "TimerQueue.h"
#include "eventsLoop.h"
#include "Timer.h"
#include <assert.h>
#include <sys/timerfd.h>
#include "TimerID.h"
#include <string.h>
#include <iostream>
TimerQueue::TimerQueue(eventsLoop *loop) : loop_(loop),
                                           timerfd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
                                           TimerfdChannel_(loop, timerfd_)
// CLOCK_MONOTONIC表示从系统重启到现在的时间，避免用户修改系统时间造成计时问题
// TFD_NONBLOCK表示获得非阻塞描述符；
{
    std::cout << "timerfd_: " << timerfd_ << std::endl;
    TimerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleReand, this));
    TimerfdChannel_.enableReading();
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry sentry = std::make_pair(now, std::shared_ptr<Timer>());
    TimerListType::iterator it = timers_.lower_bound(sentry); // 返回第一个未到期的Timer的迭代器
    assert(it == timers_.end() || now < it->first);           // Timestamp需要重载运算符
    std::copy(timers_.begin(), it, std::back_inserter(expired));
    // back_inserter返回容器的尾后迭代器，于是copy算法指定复制范围，自动从指定的第三个迭代器开始插入元素，相当于直接在expired的尾后开始插入元素
    timers_.erase(timers_.begin(), it);

    // 删除操作
    for (auto &it : expired)
    {
        activeTimerType timer(it.second);
        size_t n = activeTimersSet_.erase(timer);
        if (n != 1)
        {
            std::cout << "an error has happened in TimerQueue::getExpired\n";
            abort();
        }
    }
    assert(activeTimersSet_.size() == timers_.size());
    return expired;
}
// 调用runInLoop使得addTimer成为一个线程安全的函数，即便子线程调用到该函数，在runInLoop里也会将addTimerInLoop的工作放入代办队列pendingFunctorsList_
TimerID TimerQueue::addTimer(const TimerCallBackType & cb, Timestamp when, double interval)
{
    std::shared_ptr<Timer> timer(new Timer(cb, interval, when));
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerID(timer);
}

// 只能由IO线程完成添加定时器的工作
void TimerQueue::addTimerInLoop(std::shared_ptr<Timer> timer)
{
    loop_->assertInLoopThread();
    bool earliest = insert(timer);
    if (earliest)
    {
        // 重设第一个超时时间
        reSetTimerFd(timer->expiration(), timerfd_);
    }
}

bool TimerQueue::insert(std::shared_ptr<Timer> &timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimersSet_.size());
    Timestamp expiration = timer->expiration(); // 超时时间
    bool earliestChanged = false;
    auto it = timers_.begin();
    if (it == timers_.end() || expiration < it->first)
    {
        // 相当于应该插入到第一个位置，也就是第一个超时的定时器
        earliestChanged = true;
    }
    {
        std::pair<TimerListType::iterator, bool> result = timers_.insert(Entry(expiration, timer));
        // insert方法返回一个pair类型，first为被插入元素的迭代器类型，second为bool，如果插入成功则bool类型为true，first指向被插入元素
        assert(result.second);
    }
    {
        std::pair<activeTimerTypeSet::iterator, bool> result = activeTimersSet_.insert(activeTimerType(timer));
        assert(result.second);
    }
    assert(timers_.size() == activeTimersSet_.size());
    return earliestChanged;
}

// 重设超时时间
void TimerQueue::reSetTimerFd(Timestamp expiration, int timerfd)
{
    struct itimerspec its;
    bzero(&its, sizeof its);
    its.it_value = restTimeFromNow(expiration);
    int ret = timerfd_settime(timerfd_, 0, &its, nullptr);  //0表示相对超时，1表示绝对
    if (ret == 0)
    {
        std::cout << "timer set success\n";
    }
}

// 计算剩余超时时间
timespec TimerQueue::restTimeFromNow(Timestamp expiration)
{
    int64_t microsecons = expiration.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch(); // 得到剩余微秒
    if (microsecons < 100)
    {
        // 不足0.1ms则补足，相当于误差为0.1ms
        microsecons = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microsecons / Timestamp::kMicroSecondsPerSecond);       // 秒数
    ts.tv_nsec = static_cast<long>(microsecons % Timestamp::kMicroSecondsPerSecond * 1000); // 不足1s剩下的ns数
    return ts;
}

// 当timerfd触发超时时理论上应该调用的函数
void TimerQueue::handleReand()
{
    loop_->assertInLoopThread();
    readTimerFd();
    // 使用智能指针天然应对自我注销的情况
    // 即run运行的是cancel，返回之后引用计数也还有1，也就是在此处的vector里
    // 当本函数结束运行之后，非重复的定时器自动释放掉
    std::vector<Entry> expireds = std::move(getExpired(Timestamp::now()));
    for (auto &it : expireds)
    {
        it.second->run(); // 执行Timer的超时回调
    }
    reset(Timestamp::now(), expireds);
}

// timerfd触发超时需要read掉，不然一直触发
void TimerQueue::readTimerFd()
{
    uint64_t msg = 1;
    ssize_t nbytes = read(timerfd_, &msg, sizeof msg);
    if (nbytes != sizeof msg)
    {
        std::cout << "read error from TimerQueue::readTimerFd\n";
    }
}

//检查是否有重复触发的定时器
void TimerQueue::reset(Timestamp now, std::vector<Entry> &expireds)
{
    Timestamp nextExpire;
    for (auto &it : expireds)
    {
        if (it.second->isNeedReapeat())
        {
            it.second->restart(now);
            insert(it.second);
        }
    }
    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->first;
    }
    if (nextExpire.microSecondsSinceEpoch() > 0)
    {
        reSetTimerFd(nextExpire, timerfd_);
    }
}

void TimerQueue::cancel(TimerID tmid_)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelTimerInLoop, this, tmid_));
}

void TimerQueue::cancelTimerInLoop(TimerID tmid_)
{
    loop_->assertInLoopThread();
    activeTimerType timer(std::move(tmid_.timerPtr()));
    if(!timer) return;
    assert(activeTimersSet_.size() == timers_.size());
    auto it = activeTimersSet_.find(timer);
    if (it != activeTimersSet_.end())
    {
        activeTimersSet_.erase(it);
        timers_.erase(Entry(timer->expiration(), timer));
    }
    assert(activeTimersSet_.size() == timers_.size());
}