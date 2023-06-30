#include "IPAddr.h"
#include "socketOperations.h"
#include <string.h>
IPAddr::IPAddr(uint16_t port)
{
    bzero(&addr_ , sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = hostToNetwork32(INADDR_ANY);
    addr_.sin_port = hostToNetwork16(port);
}

IPAddr::IPAddr(const std::string & ip , uint16_t port)
{
    bzero(&addr_ , sizeof addr_);
    fromHostToNetwork(ip.c_str() , port , &addr_);
}

std::string IPAddr::toHostPort() const
{
    char buf[32];
    fromNetworktoHost(buf , sizeof buf , addr_);
    return buf;
}