#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <map>
#include <string>
#include "CallBack.h"
#include <vector>
class eventsLoopThreadPool;
class Acceptor;
class eventsLoop;
class IPAddr;
class TcpServer
{
public:
    typedef std::map<std::string , TcpConnectionPtr> ConnectionMapType;
    typedef std::function<void (const TcpConnectionPtr &)> writeCompleteCallBackType;
private:
    bool started_;
    int nextConnID_;
    const std::string name_;
    eventsLoop * loop_;
    ConnectionMapType ConnectionMap_;
    ConnectionCallBackType ConnectionCallBack_;
    MessageCallBackType MessageCallBack_;
    writeCompleteCallBackType writeCompleteCallBack_;   //低水位回调，主要方便用户在使用时可以在写缓冲为空时做特殊处理，比如可发送更多数据
    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<eventsLoopThreadPool> threadPool_;  //线程池对象

    void newConnection(int sockfd , IPAddr & Addr);
    void removeConnectionInLoop(const TcpConnectionPtr & conn);
public:
    TcpServer(eventsLoop * loop , IPAddr & , const std::string & name);
    ~TcpServer();
    void start();
    void setThreadnum(int num);
    void removeConnection(const TcpConnectionPtr & conn);
    void setConnectionCallBack(const ConnectionCallBackType & cb) {ConnectionCallBack_ = cb; }
    void setMessageCallBack(const MessageCallBackType & cb) { MessageCallBack_ = cb; }
    void setWriteComPleteCallBack(const writeCompleteCallBackType & cb) { writeCompleteCallBack_ = cb; }
    std::vector<eventsLoop *> getThreadLoop();
};



#endif