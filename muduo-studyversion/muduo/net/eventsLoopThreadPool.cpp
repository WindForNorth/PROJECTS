#include "eventsLoopThreadPool.h"
#include "eventsLoop.h"

eventsLoopThreadPool::eventsLoopThreadPool(eventsLoop *loop) :
    baseLoop_(loop),
    started_(false),
    next_(0),
    numThreads_(0)
{
    
}

eventsLoop *eventsLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    if(!eventsLoopThreads_.empty())
    {
        if(next_ == numThreads_) next_ = 0;
        return eventsLoops_[next_++ % numThreads_];
    }
    return baseLoop_;
}

void eventsLoopThreadPool::start()
{
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;
    for(int i = 0; i < numThreads_; ++i)
    {
        eventsLoopThread * t = new eventsLoopThread();
        eventsLoopThreads_.push_back(std::move(std::unique_ptr<eventsLoopThread>(t)));
        eventsLoops_.push_back(t->startLoop());
    }
}
