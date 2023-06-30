#include "../net/eventsLoop.h"
// #include <sys/timerfd.h>
// #include <string.h>
#include <string>
#include "../net/IPAddr.h"
#include "../net/TcpServer.h"
#include <iostream>
#include "../net/TcpConnection.h"
std::string msg = "";

void onConnection(const TcpConnectionPtr & conn)
{
    std::cout << "a new connection " << conn->name() << std::endl;
    conn->send("hello I am Server\n");
}
void onMessageCallBack(const TcpConnectionPtr & conn , Buffer * buff , Timestamp receveTime)
{
    std::cout << "receive " << buff->readableBytes() << " bytes from conn " << conn->name() 
        << "\nmesaage is : " << buff->retrieveTostring() << std::endl;
    conn->send("message has received\n");

}
void writeCompelete(const TcpConnectionPtr & conn){
    // conn->send(msg);
    std::cout << "msg have been sended completely\n";
}

int main(int argc , char *argv[]){
    //chargen服务器
    //将ascii字符从33号开始每次72字符一行到126号可打印字符轮换输出
    // std::string line = "";
    // for(int i = 33; i < 127; ++i){
    //     line.push_back(char(i));
    // }
    // line += line;

    // for(int i = 0; i < 127 - 33; ++i){
    //     msg += line.substr(i , 72) + "\n";
    // }

    eventsLoop loop;
    IPAddr addr(8888);
    TcpServer server(&loop , addr , "MyServer");
    server.setConnectionCallBack(onConnection);
    server.setMessageCallBack(onMessageCallBack);
    server.setWriteComPleteCallBack(writeCompelete);
    if(argc > 1){
        server.setThreadnum(atoi(argv[1]));
    }
    server.start();
    loop.loop();
    return 0;
}
