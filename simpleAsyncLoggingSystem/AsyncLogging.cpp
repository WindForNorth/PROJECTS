#include "AsyncLogging.h"
#include "LogFile.h"
/*
    线程安全的异步日志库
*/
AsyncLogging::AsyncLogging(const std::string filename , int sizePerfile , double flushinterval)
    :
    filename_(filename) , 
    filesize_(sizePerfile) , 
    flushinterval_(flushinterval),
    count_(1),
    flushThread_(std::bind(&AsyncLogging::threadFunc , this)),
    isruning_(false),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer()),
    replaceBuffer_(new Buffer()),
    dataBuffers_()
{

}

AsyncLogging::~AsyncLogging()
{
    if(isruning_) stop();
}

void AsyncLogging::append(const std::string & msg)
{
    mutexLockGuard lock(mutex_);
    if(currentBuffer_->restsize() > msg.length())
        currentBuffer_->append(msg);
    else
    {
            
        dataBuffers_.emplace_back(std::move(currentBuffer_));
        if(replaceBuffer_) currentBuffer_ = std::move(replaceBuffer_);
        else currentBuffer_.reset(new Buffer());
        currentBuffer_->append(msg);
        cond_.notify();
    }
}

void AsyncLogging::append(const char * msg , size_t size)
{
    mutexLockGuard lock(mutex_);
    if(currentBuffer_->restsize() > size)
        currentBuffer_->append(msg , size);
    else
    {
            
        dataBuffers_.emplace_back(std::move(currentBuffer_));
        if(replaceBuffer_) currentBuffer_ = std::move(replaceBuffer_);
        else currentBuffer_.reset(new Buffer());
        currentBuffer_->append(msg , size);
        cond_.notify();
    }
}

void AsyncLogging::threadFunc()
{
    assert(isruning_);
    {
        mutexLockGuard lock(mutex_);
        --count_;
        if(count_ == 0) cond_.notify();
    }
    LogFile file(filename_ , filesize_);
    std::unique_ptr<Buffer> replace1_(new Buffer());
    std::unique_ptr<Buffer> replace2_(new Buffer());
    assert(replace1_);
    assert(replace2_);
    BufferVec replaceVec;
    replace1_->reset();
    replace2_->reset();
    while(isruning_)
    {
        {
            mutexLockGuard lock(mutex_);
            if(dataBuffers_.empty())
                cond_.waitSeconds(flushinterval_);
            replaceVec.swap(dataBuffers_);
            replaceVec.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(replace1_);
            if(!replaceBuffer_) replaceBuffer_ = std::move(replace2_);
        }
        for(auto & it : replaceVec)
        {
            file.write(it->data() , it->size());
        }
        replace1_ = std::move(replaceVec.back());
        replace1_->reset();
        replaceVec.pop_back();
        if(!replace2_)
        {
            replace2_ = std::move(replaceVec.back());
            replace2_->reset();
            replaceVec.pop_back();
        }
        replaceVec.clear();
        file.flush();
    }
    again(file);
    file.flush();
}

void AsyncLogging::again(LogFile & file)
{
    mutexLockGuard lock(mutex_);
    dataBuffers_.push_back(std::move(currentBuffer_));
    for(auto & it : dataBuffers_)
    {
        file.write(it->data() , it->size());
    }
}