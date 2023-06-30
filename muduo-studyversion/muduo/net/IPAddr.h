#ifndef IPADDR_H
#define IPADDR_H
#include <netinet/in.h>
#include <string>
//IP结构和端口的封装
class IPAddr
{
private:
    struct sockaddr_in addr_;
    struct sockaddr_in6 addr6_;
public:
    IPAddr(uint16_t port);
    IPAddr(const std::string & ip , uint16_t port);
    IPAddr(const struct sockaddr_in & addr) :
        addr_(addr)
    {

    }
    IPAddr(const struct sockaddr_in6 & addr) :
        addr6_(addr)
    {

    }
    ~IPAddr() = default;
    const sockaddr_in * getSockAddr()const{ return &addr_; };
    std::string toHostPort()const;
    void setAddr(const struct sockaddr_in & addr) { addr_ = addr; }
};  



#endif