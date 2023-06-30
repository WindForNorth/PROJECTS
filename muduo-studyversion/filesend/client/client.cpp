#include "../muduo/net/eventsLoop.h"
#include "../muduo/net/IPAddr.h"
#include "../muduo/net/TcpClient.h"
#include "../muduo/net/TcpConnection.h"
#include <iostream>
char filenamebuff[100];
TcpClient *client_;
void *start(void *)
{
    eventsLoop loop;
    IPAddr addr(8888);
    TcpClient client(&loop , addr);
    client_ = &client;
    client.connect();
    loop.loop();
    return nullptr;
}

void onMessageCallBack(const TcpConnectionPtr &conn, Buffer *buff, Timestamp receiveTime)
{
    void *fp_ = conn->getAny();
    FILE *fp = static_cast<FILE *>(fp_);
    if (!fp) 
    {
        std::string str = filenamebuff;
        fp = fopen(str.substr(0 , str.size() - 1).c_str(), "wr");
        conn->setAny(static_cast<void *>(fp));
    }
    size_t n = buff->readableBytes();
    if (n > 0)
    {
        std::cout << "bytes : " << n << "\n";
        size_t bytes = fwrite(buff->peek(), 1, n, fp);
        std::cout << "write size: " << bytes << "\n";
        buff->retrieve(n);
    }
    else
    {
        fclose(fp);
        fp = nullptr;
        conn->setAny(static_cast<FILE *>(fp));
        conn->shutDown();
    }
}

int main()
{
    pthread_t tid;
    pthread_create(&tid , nullptr , start , nullptr);
    while(!client_){}
    client_->setMessageCallBack(std::bind(onMessageCallBack , 
        std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3
    ));
    int n;
    filenamebuff[0] = '.';
    filenamebuff[1] = '/'; 
    while((n = read(STDIN_FILENO , filenamebuff + 2 , sizeof filenamebuff - 2)) > 0)
    {
        TcpConnectionPtr conn = client_->getConnection();
        if(conn) conn->send(filenamebuff + 2);
    }
    return 0;
}
