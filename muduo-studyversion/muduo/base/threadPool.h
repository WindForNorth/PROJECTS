#ifndef THREADPOOL_H
#define THREADPOOL_H
#include "Thread.h"
#include "condition.h"
#include "mutex.h"
#include "noncopyable.h"
#include <vector>
#include <deque>
#include <memory>


//基于消费者的任务调用线程池
//整个线程池用于指向任务队列中的回调

class threadPool : noncopyable
{
public:
    typedef std::function<void ()> taskType;    //任务以回调的方式
private:
    size_t maxQueueSize_;
    bool running_;
    mutable mutexLock mutex_;
    condition notFull_;
    condition notEmpty_;

    std::vector<std::unique_ptr<Thread>> threadPool_;
    std::deque<taskType> taskQueue_;
    bool isFull()  { return taskQueue_.size() == maxQueueSize_; }
    bool isEmpty() { return taskQueue_.empty();}
    taskType take();
    void runInLoop();
public:
    threadPool(/* args */);
    ~threadPool();

    void setMaxQueueSize(size_t maxsize) { maxQueueSize_ = maxsize; }
    void run(taskType task);
    void start(int threadNum);  
    void stop();
    size_t quesize() const;

};





#endif