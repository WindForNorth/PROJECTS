#ifndef CONDITION_H
#define CONDITION_H

#include "mutex.h"
#include <string.h>
class condition : noncopyable
{
private:
    mutexLock & mutex_;
    pthread_cond_t cond_;
public:
    explicit 
    condition(mutexLock & mutex) :mutex_(mutex) { pthread_cond_init(&cond_  ,  nullptr); }
    ~condition() { pthread_cond_destroy(&cond_); }
    void wait() {
        //已经上锁的保证交给用户  
        mutex_.unassignHolder(); 
        pthread_cond_wait(&cond_ , mutex_.getMutex()); 
        mutex_.assignHolder();    
    }
    void waitSeconds(double waitTime) 
    {
        timespec abstime;
        bzero(&abstime , sizeof abstime);
        clock_gettime(CLOCK_REALTIME , &abstime);
        abstime.tv_nsec += waitTime * 1000000000;
        time_t secs = abstime.tv_nsec / 1000000000;
        abstime.tv_sec += secs;
        abstime.tv_nsec %= 1000000000;
        pthread_cond_timedwait(&cond_ , mutex_.getMutex() , &abstime);
    }
    void notify() { pthread_cond_signal(&cond_); }
    void notifyAll() { pthread_cond_broadcast(&cond_); }
};




#endif