#include "TcpConnection.h"
#include <unistd.h>
#include "Channel.h"
#include "eventsLoop.h"
#include <iostream>
#include "Socket.h"
#include "Buffer.h"

void TcpConnection::handleRead()
{
    int saveError = 0;
    size_t n = inputBuffer_.readfd(channel_->fd(), saveError); // 发生读事件之后在在设置只触发一次之前只有在读取完缓冲之后才能停止触发
    // 根据读取到的内容大小判断当前连接的所属情况
    if (n > 0)
    {
        if (MessageCallBack_)
            MessageCallBack_(shared_from_this(), &inputBuffer_, Timestamp::now());
    }
    else if (n == 0)
    {
        // 当对端已经关闭，我方调用read就能返回0
        // 对端调用close和shutdown(fd , SHUT_WR)
        handleClose();
    }
    else
    {
        std::cout << "some error has happened in TcpConnection::handleRead\n";
        std::cout << "error number is " << saveError << std::endl;
        handleError();
    }

    // shared_from_this()返回值是一个右值，要引用它必须使用const
}

TcpConnection::TcpConnection(
    eventsLoop *loop,
    int sockfd, // 已经建立连接的客户端套接字描述符
    std::string name,
    IPAddr &localAddr,
    IPAddr &peerAddr) : loop_(loop),
                        name_(name),
                        state_(kConnecting),
                        socket_(new Socket(sockfd)),
                        channel_(new Channel(loop, sockfd)),
                        localAddr_(localAddr), // 可以考虑实现一个右值构造
                        peerAddr_(peerAddr)
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setCloseCallBack(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    socket_->setTcpKeepAlive(true); // 开启TCP保活
}

void TcpConnection::establelished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    state_ = kConnected;
    channel_->enableReading();
    ConnectionCallBack_(shared_from_this()); // 用户连接回调
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnected || state_ == Disconnecting);
    channel_->disableAll();
    CloseCallBcak_(shared_from_this());
}

void TcpConnection::handleError()
{
    std::cout << "There is a read error\n";
}

// 说实话和handleClose太像了，但是本函数是在当前conn析构前的最后一个调用的函数
// 需要做一些善后处理，比如移除Poller的监听调用连接相关的处理回调等
// 这个函数最后会交给IO线程在doPendingFunctors内调用，当一轮pendingFunctors函数结束，这个连接conn也就被析构了
void TcpConnection::connectDestried()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnected || state_ == Disconnecting);
    setState(Disconnected);
    channel_->disableAll();
    ConnectionCallBack_(shared_from_this());
    loop_->removeChannel(channel_.get()); // 在当前主循环移除这个conn相关的Channel
}

void TcpConnection::send(const std::string &msg)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            // 如果当前就在IO线程内，就直接调用sendInLoop，减少一次函数调用
            sendInLoop(msg);
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, msg));
            // 线程安全得到了保证，换来的缺点就是会拷贝msg，因此会造成一定性能上的损耗，但是优化采用move的话就另说
        }
    }
}

void TcpConnection::send(const void *msg, size_t len)
{
    std::string msg_(static_cast<const char *>(msg), len);
    send(msg_);
}

void TcpConnection::shutDown()
{
    if (state_ == kConnected)
    {
        setState(Disconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutDownInLoop, this));
    }
}

void TcpConnection::shutDownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        socket_->shutDownWrite();
    }
}
// 在保证线程安全的情况下进行数据发送
void TcpConnection::sendInLoop(const std::string &msg)
{
    ssize_t nwrote = 0;
    ssize_t remaining = msg.size();
    bool faultHappened = false;
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        // 一定要保证当前输出缓冲区没有需要写的数据，不然此时发送新的msg就可能造成乱序
        nwrote = write(channel_->fd(), msg.data(), msg.size());
        if (nwrote >= 0)
        {
            remaining -= nwrote;
            if (remaining > 0)
            {
                std::cout << "will continue send\n";
            }
            else if (writeCompleteCallBack_)
            {
                // 说明一次数据发送完
                // 此时触发写完回调
                loop_->queueInLoop(std::bind(writeCompleteCallBack_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                std::cout << "an error hapened at TcpConnection::sendInLoop\n";
                faultHappened = true;
            }
        }
    }
    // 要考虑是否触发高水位回调和将未发送的数据append到outBuffer_
    if (!faultHappened && remaining > 0)
    {
        size_t oldBytes = outputBuffer_.readableBytes();
        // 判断是否满足触发高水位条件
        if (
            oldBytes + remaining >= highWaterMark_ &&
            oldBytes < highWaterMark_ &&
            highWaterMarkCallBack_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallBack_, shared_from_this(), oldBytes + remaining));
        }
        // append
        outputBuffer_.append(msg.data() + nwrote, remaining);
        if (!channel_->isWriting())
        {
            // 此时表示数据还没完全发送完毕，于是就需要将剩余需要发送的数据放入缓冲区中
            // 并开始关注可写事件，等到下一次在可读事件发生时回调channel的WriteCallBcak
            channel_->enableWriting();
        }
    }
}

// 一次send没有发送完，需要回调channel回调handleWrite来发送完剩余数据
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t n = write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                // 发送完缓冲区立马关闭写事件，避免忙循环
                channel_->disableWriting();
                if (state_ == Disconnecting)
                {
                    shutDownInLoop();
                }
                if (writeCompleteCallBack_)
                {
                    // 缓冲区清空也要触发这个回调
                    loop_->queueInLoop(std::bind(writeCompleteCallBack_, shared_from_this()));
                }
            }
            else
            {
                std::cout << "will continue to send message\n";
            }
        }
    }
    else
    {
        std::cout << "connection is closed , no more writing\n";
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

// void TcpConnection::setKeepAlive(bool on)
// {
//     socket_->setTcpKeepAlive(on);
// }

TcpConnection::~TcpConnection()
{
    assert(state_ == Disconnected);
    std::cout << "TcpConnection dtor at fd = " << channel_->fd() << std::endl;
    close(channel_->fd());
}
