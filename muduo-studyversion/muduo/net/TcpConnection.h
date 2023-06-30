#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H
#include <string>
#include "CallBack.h"
#include "../base/noncopyable.h"
#include "IPAddr.h"
#include "Buffer.h"
class eventsLoop;
class IPAddr;
class Channel;
class Socket;


//连接管理类
class TcpConnection : noncopyable , public std::enable_shared_from_this<TcpConnection>
{
public:
    typedef std::function<void (const TcpConnectionPtr & )> writeCompleteCallBackType;
    typedef std::function<void (const TcpConnectionPtr &  , size_t)> highWaterMarkCallBackType;
private:
    enum stateE {kConnecting , kConnected , Disconnecting , Disconnected};
    MessageCallBackType         MessageCallBack_;
    ConnectionCallBackType      ConnectionCallBack_;
    CloseCallBackType           CloseCallBcak_;
    writeCompleteCallBackType   writeCompleteCallBack_; //当数据在一次被发送完时的回调
    highWaterMarkCallBackType   highWaterMarkCallBack_; //当缓冲区的数据量首次超过highWaterMark_设置值时的回调，边沿触发，且只在上升沿触发一次
    size_t highWaterMark_;                              //高水位设置值

    eventsLoop * loop_;
    void setState(stateE state) { state_ = state; }
    std::string name_;
    stateE state_;                                      //TCPConnection基于一个已经接受连接的socket fd，因此state的初始状态就是kConnecting
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    IPAddr localAddr_;                                  //记录连接的本地地址
    IPAddr peerAddr_;                                   //记录连接对应的客户端地址
    Buffer inputBuffer_;
    Buffer outputBuffer_;

    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    //send和shutDown的内部实现
    void sendInLoop(const std::string & msg);
    void shutDownInLoop();
    void * any_;     //用户可用的一个内部指针
public:
    TcpConnection(
        eventsLoop * loop , 
        int sockfd , 
        std::string name , 
        IPAddr & localAddr , 
        IPAddr & peerAddr
        );
    ~TcpConnection();
    eventsLoop * ioLoop() { return loop_; }
    void setConnectionCallBack(const ConnectionCallBackType & cb) { ConnectionCallBack_ = cb; }
    void setMessageCallBack(const MessageCallBackType & cb) { MessageCallBack_ = cb; }
    void setCloseCallBack( const CloseCallBackType & cb) { CloseCallBcak_ = cb; }
    void setWriteCompleteCallBack( const writeCompleteCallBackType & cb) { writeCompleteCallBack_ = cb; }
    void setHighWaterMarkCallBack( const highWaterMarkCallBackType & cb , size_t waterMark) 
    { 
        highWaterMarkCallBack_ = cb;
        highWaterMark_ = waterMark; 
    }
    const IPAddr localAddr() const { return localAddr_; }
    const IPAddr peerAddr() const { return peerAddr_; }
    void connectDestried();
    void establelished();
    void send(const std::string & msg);
    void send(const void * , size_t len);
    // void prependAndsend(const std::string & msg);
    void shutDown();
    void setTcpNoDelay(bool on);    //设置是否禁用Nagle算法
    //void setKeepAlive(bool on);     //设置是否开启Tcp keepalive检测
    std::string name() { return name_; }
    bool isConnected()const { return state_ == kConnected ; }
    void setAny(void * any) { any_ = any; }
    void * getAny() { return any_; }
};


#endif