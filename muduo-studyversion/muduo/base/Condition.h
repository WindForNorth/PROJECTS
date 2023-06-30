#ifndef CONDITION_H
#define CONDITION_H

#include "mutex.h"
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
    void notify() { pthread_cond_signal(&cond_); }
    void notifyAll() { pthread_cond_broadcast(&cond_); }
};




#endif