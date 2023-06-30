#ifndef ACCEPTOR_H
#define ACCEPTOR_H
#include <functional>
#include "Socket.h"
#include "Channel.h"
#include "../base/noncopyable.h"
class eventsLoop;
class IPAddr;


//接受新TCP连接，触发事件时调用使用者的回调函数比如TcpServer
class Acceptor : noncopyable
{
public:
    typedef std::function<void (int , IPAddr&)> NewConnectionCallBackType;

private:
    NewConnectionCallBackType NewConnectionCallBack_;
    bool listening_;
    //描述符和对应的Channel，前者用于监听并接受新连接，后者负责分发其可读事件
    //回调handleRead，handleRead则调用用户的回调NewConnectionCallBack_
    Socket acceptorSocket_;
    Channel acceptorSocketChannel_;
    eventsLoop * loop_;

public:
    Acceptor(eventsLoop * loop , const IPAddr &);
    ~Acceptor();

    void handleRead();
    void setNewConnectonCallBack(const NewConnectionCallBackType & cb){ NewConnectionCallBack_ = cb; }
    void listen();
    bool listening() { return listening_; }
    int freefd_;    //用于防止文件描述符耗尽导致无法成功accept预留的文件描述符

};

#endif