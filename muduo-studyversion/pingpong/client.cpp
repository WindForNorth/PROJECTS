#include "../muduo/net/TcpClient.h"
#include "../muduo/base/Thread.h"
#include "../muduo/net/eventsLoop.h"
#include "../muduo/net/IPAddr.h"
#include "../muduo/net/eventsLoopThreadPool.h"
#include "../muduo/net/TcpConnection.h"

#include <utility>
#include <atomic>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <functional>


class Client;

class Session : noncopyable
{
 public:
  Session(eventsLoop * loop,
          const IPAddr & serverAddr,
          const std::string& name,
          Client* owner)
    : client_(loop, serverAddr),
      owner_(owner),
      bytesRead_(0),
      bytesWritten_(0),
      messagesRead_(0)
  {
    client_.setConnectionCallBack(
        std::bind(&Session::onConnection, this, std::placeholders::_1));
    client_.setMessageCallBack(
        std::bind(&Session::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  }

  void start()
  {
    client_.connect();
  }

  void stop()
  {
    client_.disconnect();
  }

  int64_t bytesRead() const
  {
     return bytesRead_;
  }

  int64_t messagesRead() const
  {
     return messagesRead_;
  }

 private:

  void onConnection(const TcpConnectionPtr& conn);

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp)
  {
    ++messagesRead_;
    bytesRead_ += buf->readableBytes();
    bytesWritten_ += buf->readableBytes();
    conn->send(buf->retrieveTostring());
  }

  TcpClient client_;
  Client* owner_;
  int64_t bytesRead_;
  int64_t bytesWritten_;
  int64_t messagesRead_;
};

class Client : noncopyable
{
 public:
  Client(eventsLoop * loop,
         const IPAddr& serverAddr,
         int blockSize,
         int sessionCount,
         int timeout,
         int threadCount)
    : loop_(loop),
      threadPool_(loop),
      sessionCount_(sessionCount),
      timeout_(timeout),
      numConnected_(0),
      numconn_(0)
  {
    loop->runAfter(timeout, std::bind(&Client::handleTimeout, this));
    if (threadCount > 1)
    {
      threadPool_.setThreadnums(threadCount);
    }
    threadPool_.start();

    for (int i = 0; i < blockSize; ++i)
    {
      message_.push_back(static_cast<char>(i % 128));
    }

    for (int i = 0; i < sessionCount; ++i)
    {
      char buf[32];
      snprintf(buf, sizeof buf, "C%05d", i);
      Session* session = new Session(threadPool_.getNextLoop(), serverAddr, buf, this);
      session->start();
      sessions_.emplace_back(session);
    }
  }

  const std::string& message() const
  {
    return message_;
  }

  void onConnect()
  {
    if (++numConnected_ == sessionCount_)
    {
      std::cout << "all connected\n";
    }
    ++numconn_;
  }

  void onDisconnect(const TcpConnectionPtr& conn)
  {
    if (--numConnected_ == 0)
    {
      std::cout << "all disconnected\n";

      int64_t totalBytesRead = 0;
      int64_t totalMessagesRead = 0;
      for (const auto& session : sessions_)
      {
        totalBytesRead += session->bytesRead();
        totalMessagesRead += session->messagesRead();
      }
      std::cout << "conn number: " << numconn_ << "\n";
      std::cout << totalBytesRead << " total bytes read\n";
      std::cout << totalMessagesRead << " total messages read\n";
      std::cout << static_cast<double>(totalBytesRead) / static_cast<double>(totalMessagesRead)
               << " average message size\n";
      std::cout << static_cast<double>(totalBytesRead) / (timeout_ * 1024 * 1024)
               << " MiB/s throughput\n";
      conn->ioLoop()->queueInLoop(std::bind(&Client::quit, this));
    }
  }

 private:

  void quit()
  {
    loop_->queueInLoop(std::bind(&eventsLoop::quit, loop_));
  }

  void handleTimeout()
  {
    std::cout << "stop\n";
    for (auto& session : sessions_)
    {
      session->stop();
    }
  }

  eventsLoop * loop_;
  eventsLoopThreadPool threadPool_;
  int sessionCount_;
  int timeout_;
  std::vector<std::unique_ptr<Session>> sessions_;
  std::string message_;
  std::atomic<int64_t> numConnected_;
  std::atomic<int64_t> numconn_;
};

void Session::onConnection(const TcpConnectionPtr& conn)
{
  if (conn->isConnected())
  {
    conn->setTcpNoDelay(true);
    conn->send(owner_->message());
    owner_->onConnect();
  }
  else
  {
    owner_->onDisconnect(conn);
  }
}

int main(int argc, char* argv[])
{
  if (argc != 7)
  {
    fprintf(stderr, "Usage: client <host_ip> <port> <threads> <blocksize> ");
    fprintf(stderr, "<sessions> <time>\n");
  }
  else
  {
    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    int threadCount = atoi(argv[3]);
    int blockSize = atoi(argv[4]);
    int sessionCount = atoi(argv[5]);
    int timeout = atoi(argv[6]);

    eventsLoop loop;
    IPAddr serverAddr(ip, port);

    Client client(&loop, serverAddr, blockSize, sessionCount, timeout, threadCount);
    loop.loop();
  }
}

