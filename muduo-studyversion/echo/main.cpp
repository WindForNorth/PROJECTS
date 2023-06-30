#include "echo.h"
#include <muduo/net/EventLoop.h>
using muduo::net::EventLoop;
using muduo::net::InetAddress;
int main(){
    EventLoop loop;
    InetAddress lisAddr(8888);
    echoServer server(&loop, lisAddr);
    server.start();
    loop.loop();
    return 0;
}