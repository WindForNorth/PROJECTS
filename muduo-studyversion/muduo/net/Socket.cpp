#include "Socket.h"
#include "IPAddr.h"
#include <netinet/in.h>
#include "socketOperations.h"
#include <netinet/tcp.h>    //for TCP_NODELAY
Socket::Socket(int sockfd) : sockfd_(sockfd)
{
    
}

void Socket::bindAddr(const IPAddr &listenAddr)
{
    bindaddr(sockfd_, listenAddr.getSockAddr());
}

void Socket::setReuseAddr(bool flag)
{
    int op = flag ? 1 : 0;
    // 地址复用可以使得同一程序可以使用相同的IP地址
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &op, sizeof op);
}

void Socket::setReusePort(bool flag)
{
    int op = flag ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &op, sizeof op);
}
int Socket::accept(IPAddr &ipaddr)
{
    struct sockaddr_in addr;
    int ret =  acceptconn(sockfd_, &addr);
    if(ret >= 0){
        //这种方式封装性更好
        ipaddr.setAddr(addr);
    }
    return ret;
}

void Socket::listen()
{
    startListen(sockfd_);
}

void Socket::shutDownWrite()
{
    shutDownwrite(sockfd_);
}

void Socket::setTcpKeepAlive(bool on)
{
    int optval = on?1:0;
    //在套接字层面上设置定期检测是否连接良好
    setsockopt(sockfd_ , SOL_SOCKET , SO_KEEPALIVE , &optval , sizeof optval);
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on?1:0;
    setsockopt(sockfd_ , IPPROTO_TCP , TCP_NODELAY , &optval , sizeof optval);
}

