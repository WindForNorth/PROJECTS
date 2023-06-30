#include "codec.h"
#include <iostream>
#include "../muduo/net/Timestamp.h"
//相当于解码，也就是检索条按照协议分割的完整信息
void codec::onMessage(
                    const TcpConnectionPtr & conn , 
                    Buffer * buff,
                    Timestamp receivetime)
{
    std::cout << "decoder\n";
    while(buff->readableBytes() > headerLen)
    {
        const void * data = static_cast<const void *>(buff->peek());
        int len = *static_cast<const int *>(data);
        if(len < 0)
        {
            std::cout << "error msglen\n";
            conn->shutDown();
            break;
        }
        else if (buff->readableBytes() >= len + headerLen)
        {
            buff->retrieve(headerLen);
            const std::string msg(buff->peek() , len);
            callBack_(msg ,  conn , receivetime);
            buff->retrieve(len);
        }
        else
        {
            break;
        }
    }
}

//相当于编码，需要发送信息的时候，借助编码器完成消息的封装
void codec::send(const TcpConnectionPtr & conn , const std::string & msg)
{
    std::cout << "encoder\n";
    int size = msg.size();
    const void * ch = static_cast<const void *>(&size);
    std::string str(static_cast<const char *>(ch) , sizeof size);
    str.append(std::move(msg));
    conn->send(str);
}