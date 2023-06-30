#ifndef CODEC_H
#define CODEC_H
#include <functional>
#include <string>
#include "../muduo/net/TcpConnection.h"
#include "../muduo/net/Buffer.h"
class codec
{
public:
    typedef std::function<void (
        const std::string & , 
        const TcpConnectionPtr &,
        Timestamp)> userMessageCallBackType;
private:
    userMessageCallBackType callBack_;
    int headerLen;
public:
    codec(const userMessageCallBackType & callback , int headerlen = 4) 
    : callBack_(callback) , headerLen(headerlen)
    {}
    ~codec() = default;
    void onMessage( const TcpConnectionPtr & conn , 
                    Buffer * ,
                    Timestamp receivetime
                    );
    void send(const TcpConnectionPtr & conn , const std::string & mag);
};



#endif