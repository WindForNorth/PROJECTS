#include "threadPool.h"

threadPool::threadPool() :
    maxQueueSize_(0),
    running_(false),
    mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_)
{

}

threadPool::~threadPool()
{
    if(running_)
    {
        stop();
    }
}
//从任务队列首部取得任务返回，可以理解为消费者线程
threadPool::taskType threadPool::take()
{
    mutexLockGuard lock(mutex_);
    if(taskQueue_.empty() && running_)
    {
        notEmpty_.wait();
    }
    taskType task;
    if(!taskQueue_.empty())
    {
        task = taskQueue_.front();
        taskQueue_.pop_front();
        if(maxQueueSize_ > 0)
        {
            notFull_.notify();
        } 
    }
    return task;
}

//使用该任务线程池的外部线程将需要交给线程池执行的任务放入队列中
void threadPool::run(taskType task)
{
    if(threadPool_.empty())
    {
        task();
    }
    else
    {
        mutexLockGuard lock(mutex_);
        while(isFull() && running_)
        {
            notFull_.wait();
        }
        if(!running_) return;
        taskQueue_.push_back(std::move(task));
        notEmpty_.notify();
    }
}

void threadPool::runInLoop()
{
    //不断从任务队列取任务执行
    while(running_)
    {
        taskType task(std::move(take()));
        if(task)
        {
            task();
        }
    }
}


void threadPool::start(int threadnum)
{
    assert(threadPool_.empty());
    running_ = true;
    threadPool_.reserve(threadnum);
    for(int i = 0 ; i < threadnum; ++i)
    {
        threadPool_.emplace_back(new Thread(std::bind(&threadPool::runInLoop , this)));
        threadPool_[i]->start();
    }
}

void threadPool::stop()
{
    {
        mutexLockGuard lock(mutex_);
        running_ = false;
        notEmpty_.notifyAll();
        notFull_.notifyAll();
    }
    for(auto & it : threadPool_)
    {
        it->join();
    }

}

size_t threadPool::quesize() const
{
    mutexLockGuard lock(mutex_);
    return taskQueue_.size();
}



