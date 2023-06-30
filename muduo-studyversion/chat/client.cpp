#include "../muduo/net/eventsLoop.h"
#include "../muduo/net/IPAddr.h"
#include "../muduo/net/TcpClient.h"
#include "codec.h"
#include <iostream>
#include <pthread.h>
codec * codec_;
eventsLoop * loop_;
TcpClient * client_;
void onMessageCallBack(
        const std::string & msg ,
        const TcpConnectionPtr & conn ,
        Timestamp receiveTime
    )
{
    std::cout << "receive " << msg.size() << " bytes from " << conn->peerAddr().toHostPort() << "\n";
    std::cout << "msg is: " << msg << "\n";
}
void * EventLoop(void *)
{
    eventsLoop loop;
    loop_ = &loop;
    IPAddr addr(8888);
    TcpClient client(&loop , addr);
    client_ = &client;
    client.setMessageCallBack(std::bind(&codec::onMessage , codec_ , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3));
    client.connect();
    std::cout << "will loop\n";
    loop_->loop();
    return nullptr;
}
int main()
{
    codec_ = (new codec(std::bind(&onMessageCallBack , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3)));
    pthread_t tid;
    pthread_create(&tid , 0 , EventLoop , nullptr);
    char buff[1024];
    int n;
    while((n = read(STDIN_FILENO , buff , sizeof buff)) > 0)
    {
        codec_->send(client_->getConnection() , std::string(buff , n));
    }
    delete codec_;
    return 0;
}