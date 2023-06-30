#include "discard.h"
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;
int main(){
    EventLoop loop;
    InetAddress inet(8888);
    discard server(&loop,inet);
    server.start();
    loop.loop();
}