#include "TcpClient.h"
#include "eventsLoop.h"
#include <string>
#include "IPAddr.h"
#include "Connector.h"
#include "socketOperations.h"
#include "TcpConnection.h"
#include "Timestamp.h"
#include "Socket.h"
#include <iostream>
TcpClient::TcpClient(eventsLoop * loop , const IPAddr & addr) :
    loop_(loop),
    connected_(false),
    retry_(false),
    name_(""),
    connector_(new Connector(loop , addr)),
    nextId_(1),
    ConnectionCallBack_(std::bind(&TcpClient::defaultConnectionCallBack , this , std::placeholders::_1)),
    MessageCallBack_(std::bind(&TcpClient::defaultMessageCallBack , 
        this , 
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3    
    ))
{
    connector_->setConnectionCallBack(std::bind(
        &TcpClient::newConnection , 
        this , 
        std::placeholders::_1));
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    IPAddr peerAddr(getpeerAddr(sockfd));
    IPAddr localAddr(getlocalAddr(sockfd));
    char buff[100];
    snprintf(buff , sizeof buff , "%s%d" , peerAddr.toHostPort().c_str() , nextId_);
    ++nextId_;
    std::string name = buff + name_;

    TcpConnectionPtr conn(new TcpConnection(loop_ , sockfd , name , localAddr , peerAddr));
    conn->setConnectionCallBack(ConnectionCallBack_);
    conn->setMessageCallBack(MessageCallBack_);
    conn->setCloseCallBack(std::bind(&TcpClient::removeConnection , this , std::placeholders::_1));
    conn->setWriteCompleteCallBack(WriteCompleteCallBack_);

    {
        MutexLockGuard lock(mutex_);
        conn_ = conn;
    }
    
    conn->establelished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->ioLoop());
    {
        MutexLockGuard lock(mutex_);
        assert(conn_ == conn);
        conn_.reset();    
    }
    loop_->queueInLoop(std::bind(&TcpConnection::connectDestried , conn));
    if(retry_ && connected_)
    {
        std::cout << "will retry\n";
        //当对端意外终止连接，比如崩溃，此时conn的Channel回调该函数，在启动自动重连时就会主动再次发起连接
        connector_->restart();
    }
}

void TcpClient::connect()
{
    assert(!connected_);
    connected_ = true;
    connector_->start();
}

void TcpClient::defaultConnectionCallBack(const TcpConnectionPtr & conn)
{
    std::cout 
    << conn->localAddr().toHostPort() 
    << "->" 
    << conn->peerAddr().toHostPort() 
    << " is success\n";
}

void TcpClient::defaultMessageCallBack(const TcpConnectionPtr & conn , 
                                        Buffer * buff , Timestamp time)
{
    buff->retriveAll();
}

void TcpClient::stop()
{
    connected_ = false;
    connector_->stop();
}

void TcpClient::disconnect()
{
    connected_ = false;
    {
        MutexLockGuard lock(mutex_);
        if(conn_) conn_->shutDown();
    }
}

TcpClient::~TcpClient()
{

}