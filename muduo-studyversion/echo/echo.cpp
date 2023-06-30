#include "echo.h"
#include <muduo/base/Logging.h>
using std::string;
echoServer::echoServer(EventLoop * loop , const InetAddress & listenAdrr) : 
    server(loop,listenAdrr,"echoServer")
{
    server.setConnectionCallback(std::bind(&echoServer::onConnection,this,std::placeholders::_1));
    server.setMessageCallback(std::bind(&echoServer::onMessage,this,
        std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3
    ));
}

void echoServer::start(){
    server.start();  //开始监听
}


void echoServer::onConnection(const TcpConnectionPtr & conn){
    LOG_INFO << "there is a new client connects " << conn->name();
}

void echoServer::onMessage(const TcpConnectionPtr & conn , Buffer * buff , Timestamp stamp){
    string str(buff->retrieveAllAsString());
    LOG_INFO << conn->name() << "has recived " << str.size() << "bytes from at " << stamp.toString();
    conn->send(str);
}
