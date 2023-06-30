#include "../net/eventsLoop.h"
#include "../net/TcpClient.h"
#include "../net/IPAddr.h"
#include "../net/TcpConnection.h"
#include <iostream>
void connectonCallBack(const TcpConnectionPtr & conn)
{
    conn->send("Hello , I am Client\n");
}


void MessageCallBack(const TcpConnectionPtr & conn , Buffer * buf, Timestamp stamp)
{
    std::cout << "receive " << buf->readableBytes() << " bytes from " 
    << conn->peerAddr().toHostPort() << "\nmessage is: " << buf->retrieveTostring() << "\n";
    conn->send("receive msg"); 
}

int main()
{
    eventsLoop  loop;
    IPAddr serverAddr("127.0.0.1" , 8888);
    TcpClient client(&loop , serverAddr);
    client.setConnectionCallBack(connectonCallBack);
    client.setMessageCallBack(MessageCallBack);
    client.enableRetry();
    client.connect();
    loop.loop();
    return 0;
}