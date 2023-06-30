#ifndef SOCKETOPERATION_H
#define SOCKETOPERATION_H
/*
    提供一些和套接字系统调用的操作
*/

class IPAddr;

struct sockaddr;
struct sockaddr_in;
#include <netinet/in.h>

int createNonBlockingSocket();
void closefd(int fd);
void bindaddr(int sockfd , const struct sockaddr_in*);
int acceptconn(int sockfd , struct sockaddr_in *);
const struct sockaddr* static_addr(const struct sockaddr_in6*);
const struct sockaddr* static_addr(const struct sockaddr_in*);
struct sockaddr* static_addr(struct sockaddr_in*);
struct sockaddr* static_addr(struct sockaddr_in6*);
inline uint16_t hostToNetwork16(uint16_t port){ return htobe16(port);}                                  //端口主机字节序到网络字节序
inline uint32_t hostToNetwork32(uint32_t ip){ return htobe32(ip); }                                     //ip地址主机字节序到网络字节序
inline uint16_t networkToHost16(uint16_t port) { return ntohs(port); }
void fromHostToNetwork(const char * ip , uint16_t port , struct sockaddr_in *);                         //一条龙转换并绑定
void fromNetworktoHost(char * buf , size_t size , const struct sockaddr_in&);                           //网络字节序转换为主机字节序
void startListen(int sockfd);
struct sockaddr_in6 getlocalAddr(int sockfd);
struct sockaddr_in6 getpeerAddr(int sockfd);
void shutDownwrite(int sockfd);
int toConnect(int sockfd , const struct sockaddr *);
int getSocketError(int sockfd);
bool isSelfConnecte(int sockfd);
#endif