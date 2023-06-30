#ifndef SOCKET_H
#define SOCKET_H
#include <sys/socket.h>
class IPAddr;


//就是一个适配器，其接口实现基本都在socketOperations文件里
class Socket
{
private:
    int sockfd_;
public:
    Socket(int sockfd);
    ~Socket() = default;
    int fd() { return sockfd_; }
    void setReuseAddr(bool flag);           //地址复用
    void setReusePort(bool flag);           //端口复用
    void bindAddr(const IPAddr &) ;
    void listen();
    int accept(IPAddr &);
    void shutDownWrite();
    void setTcpNoDelay(bool on);
    void setTcpKeepAlive(bool on);
};


#endif