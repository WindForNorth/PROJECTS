#include "socketOperations.h"
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

int createNonBlockingSocket()
{
    // IPPROTO_TCP就是指的传输控制协议
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP);
    if (sockfd < 0)
    {
        std::cout << "failed to create sockfd at socketOperations.cpp line 9\n";
    }
    return sockfd;
}

int acceptconn(int sockfd, struct sockaddr_in *addr)
{
    socklen_t socklen = sizeof *addr;
#if VALGRIND
    // 针对不支持一步创建非阻塞socket的版本
#else
    int connfd = accept4(sockfd, static_addr(addr), &socklen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if (connfd >= 0)
        return connfd;
    int saveerrno = errno;
    switch (saveerrno)
    {
        // 非致命性错误
    case EAGAIN:
    case ECONNABORTED:
    case EINTR:
    case EPROTO: // ???
    case EPERM:
    case EMFILE:
        errno = saveerrno;
        break;
    // 致命性错误
    case EBADF:
    case EFAULT:
    case EINVAL:
    case ENFILE:
    case ENOBUFS:
    case ENOMEM:
    case ENOTSOCK:
    case EOPNOTSUPP:
    {
        std::cout << "unexpected error " << errno << std::endl;
        abort();
        break;
    }
    default:
        std::cout << "unkonw error " << errno << std::endl;
        break;
    }
    return connfd;
}

const struct sockaddr *static_addr(const struct sockaddr_in6 *addr)
{
    // 为啥要多此一举，问就是static_cast没法直接转换
    return static_cast<const struct sockaddr *>((const void *)addr);
}

const struct sockaddr * static_addr(const struct sockaddr_in * addr)
{
    return static_cast<const struct sockaddr *>((const void *)addr);
}

struct sockaddr * static_addr(struct sockaddr_in6 * addr)
{
    return static_cast<struct sockaddr*>((void * )addr);
}

struct sockaddr * static_addr(struct sockaddr_in * addr)
{
    return static_cast<struct sockaddr*>((void * )addr);
}

void closefd(int fd)
{
    if (::close(fd) < 0)
    {
        std::cout << "failed to close fd " << fd << std::endl;
    }
}

void bindaddr(int sockfd, const struct sockaddr_in *addr)
{
    bind(sockfd, static_addr(addr), sizeof *addr);
}

void fromHostToNetwork(const char * ip , uint16_t port , struct sockaddr_in * addr)
{
    inet_pton(AF_INET , ip , &addr->sin_addr);
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
}

void fromNetworktoHost(char * buf , size_t size , const struct sockaddr_in & addr)
{
    char host[INET_ADDRSTRLEN];
    inet_ntop(AF_INET , &addr.sin_addr , host , sizeof host);                       //将IP地址从数值类型转换为点分十进制
    uint16_t port = networkToHost16(addr.sin_port);
    snprintf(buf , size , "%s:%u" , host , port);                                   //将完整的"IP:端口"形式以字符串放入buf
}

void startListen(int sockfd)
{
    int ret = listen(sockfd , SOMAXCONN);
    if(ret < 0)
    {
        std::cout << "listen error\n";
    }
}

struct sockaddr_in6 getlocalAddr(int sockfd)
{
    struct sockaddr_in6 localaddr;
    bzero(&localaddr , sizeof localaddr);
    socklen_t size = sizeof localaddr;
    if(getsockname(sockfd , static_addr(&localaddr) , &size) < 0)
    {
        //getsockname()获取和响应的sockfd关联的本地套接字地址结构，也就是服务器的IP和监听的端口等
        std::cout << " error at getlocalAddr\n";
    }
    return localaddr;
}

struct sockaddr_in6 getpeerAddr(int sockfd)
{
    struct sockaddr_in6 peeraddr;
    bzero(&peeraddr , sizeof peeraddr);
    socklen_t len = sizeof peeraddr;
    if(getsockname(sockfd , static_addr(&peeraddr) , &len) < 0)
    {
        std::cout << "error at getpeerAddr\n";
    }
    return peeraddr;
}

void shutDownwrite(int sockfd)
{
    if(shutdown(sockfd , SHUT_WR) < 0){
        //使用shutdown函数控制连接，行为取决于第二个参数
        //SHUT_WR关闭写，SHUT_RD关闭读，SHUT_RDWR关闭读写
        //应该注意区别它和close的不同
        std::cout << "error in socketOerations.cpp shutDownwrite\n";
    }
}

int toConnect(int sockfd , const struct sockaddr * sockaddr)
{
    return connect(sockfd , sockaddr , sizeof *sockaddr);
}

int getSocketError(int sockfd)
{
    int opval;
    socklen_t len = sizeof opval;
    if(getsockopt(sockfd , SOL_SOCKET , SO_ERROR , &opval , &len) < 0){
        return errno;
    } else {
        //调用成功则opval会存储对应套接字的error
        return opval;
    }
}

bool isSelfConnecte(int sockfd)
{
    //
    struct sockaddr_in6 localAddr6 = getlocalAddr(sockfd);
    struct sockaddr_in6 peerAddr6 = getpeerAddr(sockfd);
    if(localAddr6.sin6_family == AF_INET){
        struct sockaddr_in * localaddr4 = reinterpret_cast<struct sockaddr_in *>(&localAddr6);
        struct sockaddr_in * peeraddr4 = reinterpret_cast<struct sockaddr_in *>(&peerAddr6);
        return localaddr4->sin_port == peeraddr4->sin_port && 
        memcmp(&localaddr4->sin_addr , &peeraddr4->sin_addr , sizeof peeraddr4->sin_addr);
        
    } else if(localAddr6.sin6_family == AF_INET6){
        return  memcmp(&localAddr6.sin6_addr , &peerAddr6.sin6_addr , sizeof peerAddr6.sin6_addr) && 
        localAddr6.sin6_port == peerAddr6.sin6_port;
    }
    return false;
}