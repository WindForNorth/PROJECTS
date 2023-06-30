#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "../base/noncopyable.h"
#include "TimerID.h"
#include "TimerQueue.h"
#include "CurrentThread.h"
#include "../base/mutex.h"
#include <functional>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <memory>
#include <vector>
#include <stdio.h>
class Channel;
class Poller;

class eventsLoop : noncopyable
{
public:
    typedef std::vector<Channel *> ChannelList;
private:
    static const int ktimeOutMs;                             // poll超时时间
    const pid_t threadID_;
    bool callingPendingFunctors_;
    bool looping_;
    bool quit_;
    int wakeupfd_;                                           // 系统的eventfd，用于唤醒IO线程
    std::unique_ptr<Channel> wakeupchannel_;                 // 有fd就有其对应的Channel
    mutexLock mutex_;                                        // 保护pendingFunctorsList_
    std::vector<std::function<void()>> pendingFunctorsList_; // 待回调的functor队列，也就是queueInLoop往这个队列中放入回调函数
    std::unique_ptr<Poller> poller_;                         // 间接持有Poller
    ChannelList activeChannels_;                             // 存放从poll返回的Channel，也就是发生了事件的fd
    std::unique_ptr<TimerQueue> timersque_;                  // 定时器队列
    void abortNotInLoopThread();
    void handleRead();       
    void doPendingFunctors();                                //处理IO线程待办事项
public:
    eventsLoop(bool ifUsePoller = false);
    ~eventsLoop();
    void loop();
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }
    void updateChannel(Channel *channel);               // 更新Channel分发事件
    void removeChannel(Channel *channel);               // 移除某个Channel
    void quit()
    {
        quit_ = true;
        if (!isInLoopThread())
        {
            wakeup();                   //如果是其他线程调用了quit，为了及时退出，则需要避免IO线程在poll调用阻塞
        }
    }
    bool isInLoopThread() const { return threadID_ == currentThreadID(); }
    static eventsLoop * getCurrentLoopThread();
    void wakeup(); // 唤醒IO线程，就是通过往wakeupfd_里写点数据，让IO线程捕捉到可读事件，从poll或者epoll_wait返回

    // 定时器
    TimerID runAt(Timestamp stamp, const TimerCallBackType & cb);    // 在用户指定的时间执行某回调函数
    TimerID runAfter(double delay, const TimerCallBackType & cb);    // 在用户指定的某个时间之后执行某个回调函数（单位是s）
    TimerID runEvery(double interval, const TimerCallBackType & cb); // 在用户指定的每个间隔时间之后都会执行某个回调，相当于时间周期性函数
    void cancelTimer(TimerID tmid) { timersque_->cancel(tmid); }

    void runInLoop(const std::function<void()> &cb);   // 执行某个任务回调的接口，线程安全
    void queueInLoop(const std::function<void()> &cb); // 当其他线程调用runInLoop执行回调时，发现不是创建eventsLoop对象的线程，于是将任务回调加入队列中
};

#endif