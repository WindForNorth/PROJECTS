#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H
#include <unistd.h>
#include <syscall.h>
extern __thread int t_cachedThreadID;

inline int currentThreadID()
{
    if(__builtin_expect(t_cachedThreadID == 0 , 0))
    {
        t_cachedThreadID = syscall(__NR_gettid);
    }
    return t_cachedThreadID;
}

#endif