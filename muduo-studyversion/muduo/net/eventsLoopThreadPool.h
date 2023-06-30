#ifndef EVENTSLOOPTHREADPOOL_H
#define EVENTSLOOPTHREADPOOL_H
#include "../base/noncopyable.h"
#include "eventsLoopThread.h"
#include <vector>
#include <memory>

class eventsLoop;
class eventsLoopThreadPool : noncopyable
{
private:
    int numThreads_;
    int next_;
    bool started_;
    eventsLoop * baseLoop_;
    std::vector<std::unique_ptr<eventsLoopThread>>eventsLoopThreads_;
    std::vector<eventsLoop*>eventsLoops_;
public:
    eventsLoopThreadPool(eventsLoop * );
    ~eventsLoopThreadPool() = default;

    void start();
    void setThreadnums(int theadnums) { numThreads_ = theadnums; }
    std::vector<eventsLoop*> getThreadLoop() { return eventsLoops_ ; }
    eventsLoop * getNextLoop(); 
    
};


#endif