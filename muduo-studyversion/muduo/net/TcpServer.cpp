#include "TcpServer.h"
#include "eventsLoop.h"
#include "IPAddr.h"
#include "TcpConnection.h"
#include "socketOperations.h"
#include "Acceptor.h"
#include "eventsLoopThreadPool.h"
#include  <iostream>
#include <functional>
TcpServer::TcpServer(eventsLoop * loop , IPAddr & Addr , const std::string & name):
    started_(false),
    nextConnID_(0),
    name_(name),
    loop_(loop),
    acceptor_(new Acceptor(loop , Addr)),
    threadPool_(new eventsLoopThreadPool(loop))
{
    acceptor_->setNewConnectonCallBack(
        std::bind(&TcpServer::newConnection , this , 
        std::placeholders::_1,
        std::placeholders::_2
        ));
}

std::vector<eventsLoop*> TcpServer::getThreadLoop() { return threadPool_->getThreadLoop(); }
//Acceptor的回调
void TcpServer::newConnection(int sockfd , IPAddr & Addr)
{
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf , sizeof buf , "Client#%d" , nextConnID_);     //引用nextConnID的字面
    ++nextConnID_;
    std::string newConnName = name_ + buf;
    std::cout << "a new Connction: " << newConnName << " has come from " << Addr.toHostPort() << std::endl;
    /*
        localAddr 和 peerAddr的区别，前者属于sockfd和本地绑定的某个套接字地址结构，后者属于与远端（即客户端）关联的套接字地址结构
    */

    IPAddr localAddr(getlocalAddr(sockfd));
    std::cout << "new clientfd: " << sockfd << "\n";
    eventsLoop * ioLoop = threadPool_->getNextLoop();           //获取IO线程池的指针
    TcpConnectionPtr conn(new TcpConnection(ioLoop , sockfd , newConnName , localAddr , Addr));
    ConnectionMap_[newConnName] = conn;                         //新增连接
    conn->setConnectionCallBack(ConnectionCallBack_);           //ConnectionCallBack_将在conn的establed时调用
    conn->setMessageCallBack(MessageCallBack_);                 //在conn的handleRead内调用
    conn->setCloseCallBack(std::bind(&TcpServer::removeConnection , this , std::placeholders::_1));
    conn->setWriteCompleteCallBack(writeCompleteCallBack_);
    //conn->establelished();
    //利用runInLoop的线程安全性将新连接放入另一个IO线程
    //当然如果getNextLoop返回的就是本线程（也就是单线程的情况），这个调用的功能和注释掉的没区别                                      
    ioLoop->runInLoop(std::bind(&TcpConnection::establelished , conn)); //调用TCPServer连接回调函数，即用户设定的回调函数

}

void TcpServer::start()
{
    if(!started_)
    {
        started_ = true;
        threadPool_->start();
    }
    if(!acceptor_->listening())
    {
        loop_->runInLoop(std::bind(&Acceptor::listen , acceptor_.get())); 
        // acceptor_->listen();
    }
}

void TcpServer::removeConnection(const TcpConnectionPtr & conn)
{
    //该函数只能在拥有TCPServer对象的IO线程调用
    //而当这个对象被多个线程共享的时候就可能出现其他线程调用removeConnectionInLoop的情况，因此必须防止这种情况发生
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop , this , conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr & conn)
{
    loop_->assertInLoopThread();
    std::cout << "remove a conn " << conn->name() << "\n";
    size_t n = ConnectionMap_.erase(conn->name());
    assert(n == 1);
    eventsLoop * ioLoop = conn->ioLoop();   //将销毁移交到其所属的IO线程
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestried , conn));  //直接指定参数，会拷贝一份，于是当IO线程执行完pendingFunctors就会被析构
}

TcpServer::~TcpServer()
{
    
}

void TcpServer::setThreadnum(int num)
{
    assert(num >= 0);
    threadPool_->setThreadnums(num);
}