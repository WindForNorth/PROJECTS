#ifndef ECHO_H
#define ECHO_H

#include <muduo/net/TcpServer.h>
using muduo::net::TcpServer;
using muduo::net::EventLoop;
using muduo::net::InetAddress;
using muduo::net::TcpConnectionPtr;
using muduo::net::Buffer;
using muduo::Timestamp;
class echoServer{
    public:
        echoServer(EventLoop * loop , const InetAddress & listenAddr);
        void start();
    private:
        TcpServer server;
        void onConnection(const TcpConnectionPtr&);
        void onMessage(const TcpConnectionPtr&, Buffer * ,Timestamp stamp);
};

#endif