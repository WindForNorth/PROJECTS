#ifndef THREAD_H
#define THREAD_H
#include <functional>
#include <iostream>
#include <pthread.h>
class Thread
{
public:
    typedef std::function<void()> threadFuncType;
private:
    bool started_;                              //标识线程是否已经启动
    bool joined_;                               //标识该线程所属的进程的主线程是否正在退出
    threadFuncType ThreadCallBack_;
    threadFuncType threadInitCallBack;
    pthread_t pthreadID_;
public:
    Thread(threadFuncType tf);
    ~Thread();

    bool started(){ return started_; }
    void start();
    int join();
};

//帮助存储一些线程相关数据，同时可以增大扩展性
struct ThreadData{
    Thread::threadFuncType func_;
    std::string tidName_;                       //线程名字
    pid_t tid_;                                 //线程唯一标识
    ThreadData(Thread::threadFuncType tf , std::string name = "thread"):
        func_(tf),
        tidName_(name)
    {

    }
    void run();
};
#endif