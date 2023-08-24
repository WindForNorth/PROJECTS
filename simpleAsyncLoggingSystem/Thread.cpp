#include "Thread.h"
#include <assert.h>
#include <memory>
#include <syscall.h>
#include <unistd.h>
#include "../net/CurrentThread.h"
Thread::Thread(threadFuncType tf) :
    ThreadCallBack_(tf),
    started_(false),
    joined_(false)
{

}


void * startThread(void * obj){
    ThreadData * data = static_cast<ThreadData*>(obj);
    data->run();
    delete data;
    return nullptr;
}

void Thread::start(){
    assert(!started_);
    started_ = true;
    ThreadData * data = new ThreadData(ThreadCallBack_);
    if(pthread_create(&pthreadID_ , 0 , &startThread , data)){
        //返回大于0就是errno，表示创建失败
        started_ = false;
        delete data;
        std::cout << "failed to create a thread\n";
    } else {
        //返回0就是创建成功了
        //暂时return
        return;
    }

}

void ThreadData::run(){
    tid_ = currentThreadID();
    //暂时只是回调函数
    func_();
}


Thread::~Thread(){
    if(started_ && !joined_){
        //主线程没准备退出，但是却有子线程的对象在析构
        pthread_detach(pthreadID_);     //线程分离
    }
}

int Thread::join(){
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadID_ , nullptr);      //等待子线程结束
}