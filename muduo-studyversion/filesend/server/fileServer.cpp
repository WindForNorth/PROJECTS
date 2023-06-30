#include "../muduo/net/eventsLoop.h"
#include "../muduo/net/IPAddr.h"
#include "../muduo/net/TcpServer.h"
#include "../muduo/net/TcpConnection.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
int bytes = 0;
const int STACKBUFFERSIZE = 1024 * 64; // 64KB
class fileServer
{
private:
    TcpServer server_;
    void onConnectionCallBack(const TcpConnectionPtr &conn);
    void onWriteCompleteCallBack(const TcpConnectionPtr &conn);
    void onMessageCallBack(const TcpConnectionPtr &conn, Buffer *buff, Timestamp receiveTime);

public:
    fileServer(eventsLoop *loop, IPAddr &addr, int threadnum = 4);
    ~fileServer();
    void start()
    {
        server_.start();
        server_.setWriteComPleteCallBack(std::bind(&fileServer::onWriteCompleteCallBack, this,
                                                   std::placeholders::_1));
    }
};

fileServer::fileServer(eventsLoop *loop, IPAddr &addr, int threadnum) : server_(loop, addr, "fileServer")
{
    server_.setThreadnum(threadnum);
    server_.setMessageCallBack(std::bind(&fileServer::onMessageCallBack, this,
                                         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    server_.setConnectionCallBack(std::bind(&fileServer::onConnectionCallBack, this,
                                            std::placeholders::_1));
}

fileServer::~fileServer()
{
}

void fileServer::onConnectionCallBack(const TcpConnectionPtr &conn)
{
}

void fileServer::onWriteCompleteCallBack(const TcpConnectionPtr &conn)
{
    void *fp_ = conn->getAny();
    FILE *fp = static_cast<FILE *>(fp_);
    if (fp)
    {
        char buff[STACKBUFFERSIZE];
        size_t n = fread(buff, 1, STACKBUFFERSIZE, fp);
        if (n > 0)
        {
            conn->send(buff, n);
            bytes += n;
            std::cout << "bytes: " << n << "\n";
        }
        else
        {
            fclose(fp);
            fp = nullptr;
            conn->setAny(fp);
            // conn->shutDown();
            std::cout << "all bytes size:" << bytes << "\n";
        }
    }
}

void fileServer::onMessageCallBack(const TcpConnectionPtr &conn, Buffer *buff, Timestamp receiveTime)
{
    void *fp_ = conn->getAny();
    FILE *fp = static_cast<FILE *>(fp_);
    if (!fp)
    {
        std::string filename = "./" + buff->retrieveTostring();
        fp = fopen(filename.substr(0 , filename.size() - 1).c_str(), "r");
        char buff_[STACKBUFFERSIZE];
        if (fp)
        {
            size_t n = fread(buff_, 1, STACKBUFFERSIZE, fp);
            if (n > 0)
            {
                bytes += n;
                conn->send(buff_, n);
            }
            conn->setAny(static_cast<void *>(fp));
        }
        else
        {
            std::cout << strerror(errno) << "\n";
        }
    }
}

int main()
{
    eventsLoop loop;
    IPAddr addr(8888);
    fileServer server(&loop, addr);
    server.start();
    loop.loop();
}

