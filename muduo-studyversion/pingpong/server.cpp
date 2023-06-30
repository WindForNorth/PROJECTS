#include "../muduo/net/TcpServer.h"

#include "muduo/base/Atomic.h"
#include "../muduo/base/Thread.h"
#include "../muduo/net/eventsLoop.h"
#include "../muduo/net/IPAddr.h"
#include "../muduo/net/TcpConnection.h"
#include <utility>

#include <stdio.h>
#include <unistd.h>

void onConnection(const TcpConnectionPtr& conn)
{
  if (conn->isConnected())
  {
    conn->setTcpNoDelay(true);
  }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp)
{
  conn->send(buf->retrieveTostring());
}

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    fprintf(stderr, "Usage: server <address> <port> <threads>\n");
  }
  else
  {
    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    IPAddr listenAddr(ip, port);
    int threadCount = atoi(argv[3]);

    eventsLoop loop;

    TcpServer server(&loop, listenAddr, "PingPong");

    server.setConnectionCallBack(onConnection);
    server.setMessageCallBack(onMessage);

    if (threadCount > 1)
    {
      server.setThreadnum(threadCount);
    }

    server.start();

    loop.loop();
  }
}

