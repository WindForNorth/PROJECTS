#ifndef MUTEX_H
#define MUTEX_H
#include <pthread.h>
#include <assert.h>
#include "noncopyable.h"
#include "../net/CurrentThread.h"
class mutexLock : noncopyable
{
private:
    friend class condition;
    pthread_mutex_t mutex_;
    pid_t holder_;

    void unassignHolder() { holder_ = 0; }
    void assignHolder() { holder_ = currentThreadID(); }
public:
    mutexLock() : holder_(0)
    {
        pthread_mutex_init(&mutex_ , nullptr);
    }
    ~mutexLock()
    {
        assert(holder_ == 0);
        pthread_mutex_destroy(&mutex_);
    }
    void lock() { pthread_mutex_lock(&mutex_); assignHolder();}
    void unlock() { unassignHolder(); pthread_mutex_unlock(&mutex_); }
    bool isLockedByThisThread() { return holder_ == currentThreadID(); }
    pthread_mutex_t * getMutex() { return &mutex_; }
};

class mutexLockGuard
{
private:
    mutexLock & lock_;
public:
    mutexLockGuard(mutexLock & lock) : lock_(lock)
    {
        lock_.lock();
    }
    ~mutexLockGuard()
    {
        lock_.unlock();
    }
};
#define mutexLockGuard(x) static_assert(false , "miss lock")

#endif