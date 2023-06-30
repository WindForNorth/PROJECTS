#ifndef DISCARD_H
#define DISCARD_H

#include <muduo/net/TcpServer.h>
using namespace muduo;
using namespace muduo::net;
class discard{
public:
    discard(EventLoop * loop , InetAddress & Inet);

    void start();//留给客户的启动接口

private:
    //事件回调函数
    //即用于向库注册的发生新连接事件和消息数据到达事件是的应对方式
    void onConnection(const TcpConnectionPtr & conn);
    void onMessage(const TcpConnectionPtr & conn,Buffer * buff,Timestamp stamp);
    TcpServer server;
};

#endif