#ifndef EVENTSLOOPTHREAD_H
#define EVENTSLOOPTHREAD_H

#include "../base/condition.h"
#include "../base/mutex.h"
#include "../base/Thread.h"
#include "../base/noncopyable.h"
class eventsLoop;

//IO线程库
class eventsLoopThread : noncopyable
{
private:
    mutexLock mutex_;
    condition cond_;
    Thread thread_;
    bool exiting_;
    eventsLoop * loop_;
    void threadMainFunc();
public:
    eventsLoop * startLoop();
    eventsLoopThread(/* args */);
    ~eventsLoopThread();
};



#endif