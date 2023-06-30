#include "discard.h"
#include <muduo/base/Logging.h>
#include <iostream>
#include <functional>
using namespace muduo;
using namespace muduo::net;
using std::cout;
using std::endl;
discard::discard(EventLoop * loop,
            InetAddress & Inet) : 
            server(loop,Inet,"discard")
            {
                //设置回调函数
                //注意这个TcpConnectionPtr类型的在回调函数中作为参数传递时，最好是const的引用，不然bind出错，最重要的是必须为const
                //应该是回调时实参是const的，如果形参是非const无法完成转换
                  server.setConnectionCallback(std::bind(&discard::onConnection,this,_1));
                  server.setMessageCallback(std::bind(&discard::onMessage,this,_1,_2,_3));
            }

void discard::start(){
    server.start();
}        

void discard::onConnection(const TcpConnectionPtr & conn){
    cout << "there is a new client " << conn->peerAddress().toIpPort() << endl;
}

void discard::onMessage(const TcpConnectionPtr & conn,
                        Buffer * buff,
                        Timestamp stamp )
                    {
                        string str(buff->retrieveAllAsString());
                        cout << conn->name() << "discards" << str.size() << "bytes received at " << stamp.toString() << endl;
                    }


