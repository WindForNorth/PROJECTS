#include "Connector.h"
#include "eventsLoop.h"
#include "socketOperations.h"
#include <string.h>
#include <iostream>
#include <random>

const double Connector::maxRetryDelayMs_ = 30.0 * 1000.0;
const double Connector::retryInitMs_ = 500.0;
const int Connector::maxRetryTimes_ = 9;

Connector::Connector(eventsLoop * loop , const IPAddr & Addr) :
    loop_(loop),
    Addr_(Addr),
    connect_(false),
    retryMs_(retryInitMs_),
    state_(kDisconnected),
    retryTimes_(0)
{
    
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop , this));
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if(connect_){
        connect();
    } else {
        std::cout << "do not start\n";
    }
}

void Connector::connect()
{
    int sockfd = createNonBlockingSocket();
    int ret = toConnect(sockfd , static_addr(Addr_.getSockAddr()));  //非阻塞connect
    int saveError = ret == 0?0:errno;
    switch(saveError)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
        {
            connecting(sockfd);
            break;
        }
        case EAGAIN:
        case ENETUNREACH:
        case ECONNREFUSED:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        {
            retry(sockfd);
            break;
        }
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case ENOTSOCK:
        case EFAULT:
        {
            std::cout << "error happened: " << strerror(saveError) << "\n";
            closefd(sockfd);
            break;
        }
        default:
        {
            std::cout << "unexpect error happened\n";
            closefd(sockfd);
            break;
        }

    }
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_ , sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite , this));
    channel_->setErrCallback(std::bind(&Connector::handleError , this));
    channel_->enableWriting();
}

void Connector::retry(int sockfd)
{
    closefd(sockfd);
    setState(kDisconnected);
    if(connect_){
        std::cout << "will retry after " << retryMs_ << " ms\n";
        if(++retryTimes_ > maxRetryTimes_) return;
        loop_->runAfter(retryMs_ / 1000.0 , std::bind(&Connector::startInLoop , shared_from_this()));
        retryMs_ = std::min(retryMs_ * 2 , maxRetryDelayMs_);
    } else {
        std::cout << "do not start\n";
    }
}

void Connector::handleWrite()
{
    
    if(state_ == kConnecting){
        //此时处于发起连接之后的第一次可写事件回调
        //需要在这里进行连接是否真正建立的逻辑判断，确保使用的sockfd是真正完成连接建立的
        int sockfd = removeAndResetChannel();
        int err = getSocketError(sockfd);
        if(err){
            std::cout << "connect error: " << strerror(err) << "\n";
            retry(sockfd);
        } else if(isSelfConnecte(sockfd)){
            //自连接
            std::cout << "self connect\n";
            retry(sockfd);
        } else {
            //成功建立
            //调用连接回调
            setState(kConnected);
            retryTimes_ = 0;
            newConnectionCallBack_(sockfd);
        }
    } else {
        assert(state_ == kDisconnected);
    }
    
}

void Connector::handleError()
{
    //报告错误并重启
    std::cout << "Connector handle Error\n";
    if(state_ == kConnecting){
        int sockfd = removeAndResetChannel();
        int err = getSocketError(sockfd);
        std::cout << "err discription: " << strerror(err) << "\n";
        setState(kDisconnected);
        retry(sockfd);
    }
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    //这里是不能直接reset的，因为现在可能还在Channel内部调用handle事件
    //并且Channel的回调和代办都是在IO线程内调用，因此也不用担心多线程会抢先一步执行reset导致出错
    //也就是说当前就是在IO线程内，因为handleWrite调用该函数
    loop_->queueInLoop(std::bind(&Connector::resetChannel , this));
    return sockfd;
}

void Connector::resetChannel()
{
    //由IO线程调用
    channel_.reset();   //放空
}

//让Connector能够复用
//想要让其能够复用，则Connector不能和任何
void Connector::restart()
{
    loop_->assertInLoopThread();
    connect_ = true;
    retryTimes_ = 0;
    setState(kDisconnected);
    //重连的时长应该具有随机性，不然多个连接同时发起重连容易丢包并且对服务器短时间造成较大压力
    std::default_random_engine e;
    std::uniform_real_distribution<double>u(1 , 4);
    retryMs_ = retryInitMs_ * u(e);     //使用伪随机每次均匀生成1~4之间的浮点数，也就是将延迟随机到0.5~2.0s
    startInLoop();
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop , this));
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if(state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        setState(kDisconnected);
        retry(sockfd);
    }
}

