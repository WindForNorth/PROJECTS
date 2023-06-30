#ifndef CALLBACK_H
#define CALLBACK_H
#include <functional>
#include <memory>
class TcpConnection;
class Buffer;
class Timestamp;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void (const TcpConnectionPtr &)> ConnectionCallBackType;
typedef std::function<void (const TcpConnectionPtr & , Buffer *, Timestamp)> MessageCallBackType;
typedef std::function<void ()> TimerCallBackType;
typedef std::function<void (const TcpConnectionPtr &)> CloseCallBackType;


#endif