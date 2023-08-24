#ifndef  ASYNCLOGGING_H
#define  ASYNCLOGGING_H
#include <atomic>
#include <string>
#include <vector>
#include "Thread.h"
#include "condition.h"
#include <string.h>
#include <memory>
class LogFile;
const int FIXEDBUFFSIZE = 1024 * 1024;


template <int BUFFSIZE>
class FixedBuffer : noncopyable
{
private:
    char * curidx_;
    char data_[BUFFSIZE];
public:
    FixedBuffer () : curidx_(data_)
    {

    }

    ~FixedBuffer() {}
    void append(const std::string & msg)
    {
        int restsize = (data_ + sizeof data_) - curidx_;
        if(msg.length() > restsize) return;
        memcpy(curidx_ , msg.c_str() , msg.length());
        curidx_ += msg.length();
    }
    void append(const char * msg , size_t size)
    {
        int restsize = (data_ + sizeof data_) - curidx_;
        if(size > restsize) return;
        memcpy(curidx_ , msg , size);
        curidx_ += size;
    }
    const char * data() { return data_; }
    void reset() { curidx_ = data_; clear(); }
    void clear() { bzero(data_ , sizeof data_); }
    int size() { return curidx_ - data_ ;}
    const char * curidx_inbuff() { return curidx_; }
    size_t restsize() { return (sizeof(data_) + data_) - curidx_inbuff(); }
};




class AsyncLogging
{
public:
    typedef FixedBuffer<FIXEDBUFFSIZE> Buffer;
    typedef std::vector<std::unique_ptr<Buffer>> BufferVec;
private:
    double flushinterval_;
    const std::string filename_;
    const long filesize_;
    int count_;
    Thread flushThread_;
    std::atomic<bool> isruning_;
    mutexLock mutex_;
    condition cond_;
    std::unique_ptr<Buffer> currentBuffer_;
    std::unique_ptr<Buffer> replaceBuffer_;
    BufferVec dataBuffers_;


    void threadFunc();
    void again(LogFile & file);
public:
    AsyncLogging(const std::string filename , int sizePerfile , double flushinterval);
    ~AsyncLogging();
    void append(const std::string & msg);
    void append(const char * msg , size_t size);
    void start()
    {
        isruning_ = true;
        flushThread_.start();
        {
            mutexLockGuard lock(mutex_);
            while(count_ > 0)
                cond_.wait(); //可能会是子线程先被调度，因此此处用一个变量判断子线程是否已经启动，然后在没有启动时阻塞等待子线程唤醒，也就是说要确保子线程正常启动
        }
    }
    void stop()
    {
        isruning_ = false;
        cond_.notify();
        flushThread_.join();
    }
};

#endif