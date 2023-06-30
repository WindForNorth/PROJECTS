#ifndef TCPCLIENT_H
#define TCPCLIENT_H
#include "CallBack.h"
#include <string>
#include "../base/Mutex.h"
class Connector;
class eventsLoop;
class IPAddr;
class Buffer;
class Timestamp;

class TcpClient
{
public:
    typedef std::shared_ptr<Connector> ConnectorPtr;
    typedef std::function<void(const TcpConnectionPtr &)> WriteCompleteCallBackType;
private:
    eventsLoop * loop_;
    ConnectorPtr connector_;
    MutexLock mutex_;
    TcpConnectionPtr conn_ GUARDED_BY(mutex_);
    bool retry_;
    bool connected_;
    int nextId_;
    const std::string  name_;

    //下面俩要有默认处理
    ConnectionCallBackType ConnectionCallBack_;
    MessageCallBackType MessageCallBack_;
    WriteCompleteCallBackType WriteCompleteCallBack_;
    
    void newConnection(int sockfd);                         //作为Connector的回调
    void removeConnection(const TcpConnectionPtr & conn);
public:
    TcpClient(eventsLoop * loop , const IPAddr & addr);
    ~TcpClient();

    void setConnectionCallBack(const ConnectionCallBackType & cb) { ConnectionCallBack_ = cb; }
    void setMessageCallBack(const MessageCallBackType & cb) { MessageCallBack_ = cb; }
    void setwriteCompleteCallBack(const WriteCompleteCallBackType & cb) { WriteCompleteCallBack_ = cb; }
    void connect();
    void disconnect();
    void stop();
    bool retry() { return retry_; }
    void enableRetry() { retry_ = true; }
    TcpConnectionPtr getConnection() { return conn_; }
    void defaultConnectionCallBack(const TcpConnectionPtr &);
    void defaultMessageCallBack(const TcpConnectionPtr & , Buffer * , Timestamp);
};




#endif