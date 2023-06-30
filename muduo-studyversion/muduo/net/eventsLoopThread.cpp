#include "eventsLoopThread.h"
#include <functional>
#include "eventsLoop.h"
eventsLoopThread::eventsLoopThread() :
    exiting_(false),
    thread_(std::bind(&eventsLoopThread::threadMainFunc , this)),
    mutex_(),
    cond_(mutex_),
    loop_(nullptr)
{

}

eventsLoop * eventsLoopThread::startLoop(){
    assert(!thread_.started());
    thread_.start();
    {
        //在多线程下，指不定哪个线程会被先调度，因此，为了确保在该线程使用loop对象前它已经被创建，需要在其没有被创建之前阻塞等待
        //因此使用条件变量的方式等待创建loop对象的线程唤醒
        mutexLockGuard lock(mutex_);
        while(!loop_){
            cond_.wait();
        }
    }
    return loop_;           //使得调用这个函数的线程就不是IO线程
}

//这个是IO线程的回调，也就是新建的那个线程
void eventsLoopThread::threadMainFunc(){
    eventsLoop loop;        //创建loop对象就相当于成为了IO线程，但是确实是子线程
    {
        mutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();     //唤醒阻塞线程，也就是调用上面那个函数的线程
    }
    loop.loop();            //开始主循环
}

eventsLoopThread::~eventsLoopThread(){
    exiting_ = true;
    loop_->quit();          //主线程退出就结束IO线程
    thread_.join();
}


