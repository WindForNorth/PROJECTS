#include "codec.h"
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include "../muduo/net/eventsLoop.h"
#include "../muduo/net/TcpServer.h"
#include "../muduo/net/IPAddr.h"
#include "../muduo/base/mutex.h"

class chatServer
{
private:
    TcpServer server_;
    codec codec_;
    mutexLock mutex_;
    std::vector<eventsLoop *> threadLoops_;
    std::map<eventsLoop * , std::set<TcpConnectionPtr>> connMap_;
    void onMessageCallBack(const std::string & msg , const TcpConnectionPtr & , Timestamp receiveTime);
    void onConnectionCallBack(const TcpConnectionPtr & );
    void distributeMsg(const std::string & msg , eventsLoop *);
public:
    chatServer(eventsLoop * loop , IPAddr & addr);
    ~chatServer();
    void start() 
    {
        server_.setThreadnum(4);
        server_.start();
        threadLoops_ = std::move(server_.getThreadLoop());
    }
};

chatServer::chatServer(eventsLoop * loop , IPAddr & addr) :
    server_(loop , addr , "chatServer"),
    codec_(std::bind(
        &chatServer::onMessageCallBack , this , 
        std::placeholders::_1, std::placeholders::_2 , std::placeholders::_3
    )),
    mutex_()
{
    server_.setMessageCallBack(std::bind(&codec::onMessage , &codec_ , 
        std::placeholders::_1, std::placeholders::_2 , std::placeholders::_3
    ));
    server_.setConnectionCallBack(std::bind(&chatServer::onConnectionCallBack , this , 
        std::placeholders::_1
    ));

}
chatServer::~chatServer()
{

}

void chatServer::onMessageCallBack(const std::string & msg , const TcpConnectionPtr &conn , Timestamp receiveTime)
{
    mutexLockGuard lock(mutex_);
    for(auto  loop : threadLoops_)
    {

        if(!connMap_[loop].empty())
        {
            loop->queueInLoop(std::bind(&chatServer::distributeMsg , this , msg , loop));
        }
        std::cout << "loop# " << loop << "\n";
    }
}

void chatServer::onConnectionCallBack(const TcpConnectionPtr & conn)
{
    if(conn->isConnected())
    {
        std::cout << "a new conn from " << conn->peerAddr().toHostPort() << "\n";
        eventsLoop * baseLoop = conn->ioLoop();
        mutexLockGuard lock(mutex_);
        connMap_[baseLoop].insert(conn);
    }
    else
    {
        eventsLoop * baseLoop = conn->ioLoop();
        mutexLockGuard lock(mutex_);
        connMap_[baseLoop].erase(conn);
    }
}

void chatServer::distributeMsg(const std::string & msg , eventsLoop * loop)
{
    for(auto & it : connMap_[loop])
    {
        codec_.send(it , msg);
    }
}

int main()
{
    eventsLoop loop;
    IPAddr addr(8888);
    chatServer server(&loop , addr);
    server.start();
    loop.loop();
}