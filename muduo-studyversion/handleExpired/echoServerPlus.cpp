#include "../muduo/net/eventsLoop.h"
#include "../muduo/net/TcpServer.h"
#include "../muduo/net/TcpConnection.h"
#include <iostream>
#include <list>

class echoServer
{
private:
    TcpServer server_;
    typedef std::weak_ptr<TcpConnection> weakTcpconnectionPtr;
    typedef struct Node
    {
        Timestamp lastReceiveTime;
        weakTcpconnectionPtr conn;
        std::list<struct Node>::iterator position;
    } listNodeType;
    std::list<listNodeType> connectionList_;
    const int expiredTime;
public:
    echoServer(eventsLoop *loop, IPAddr &addr, int expired);
    ~echoServer();

    void onConnection(const TcpConnectionPtr &conn);
    void onMessage(
        const TcpConnectionPtr &conn,
        Buffer *buff,
        Timestamp stamp);
    void start() { server_.start(); }
    void onTimer();
};

echoServer::echoServer(eventsLoop *loop, IPAddr &addr, int expired) : server_(loop, addr, "echoServer"),
                                                                      expiredTime(expired)
{
    loop->runEvery(1.0, std::bind(&echoServer::onTimer, this));
    server_.setConnectionCallBack(std::bind(
        &echoServer::onConnection,
        this,
        std::placeholders::_1));
    server_.setMessageCallBack(std::bind(
        &echoServer::onMessage, this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3));
}

echoServer::~echoServer()
{
}

void echoServer::onConnection(const TcpConnectionPtr &conn)
{
    bool flag = conn->isConnected();
    std::cout << "connection '" << conn->name()
              << "' " << conn->localAddr().toHostPort()
              << "->" << conn->peerAddr().toHostPort()
              << " is " << (flag ? "UP\n" : "DOWN\n");

    if (flag)
    {
        listNodeType node;
        node.lastReceiveTime = Timestamp::now();
        node.conn = conn;
        connectionList_.emplace_back(node);
        auto it = --connectionList_.end();
        it->position = it;
        conn->setAny(static_cast<void *>(&*it));
    }
    else
    {
        // 此时是内部断开连接最后销毁前调用回调
        listNodeType *node = static_cast<listNodeType *>(conn->getAny());
        assert(node);
        connectionList_.erase(node->position);
    }
}

void echoServer::onMessage(
    const TcpConnectionPtr &conn,
    Buffer *buff,
    Timestamp receiveTime)
{
    size_t n = buff->readableBytes();
    std::string msg(buff->retrieveTostring());
    std::cout << "receive " << n << " bytes from "
              << conn->peerAddr().toHostPort() << "\n";
    conn->send(msg);

    listNodeType *node = static_cast<listNodeType *>(conn->getAny());
    assert(node);
    auto it = node->position;
    node->lastReceiveTime = receiveTime;
    connectionList_.splice(connectionList_.end(), connectionList_, it);
    assert(node->position == --connectionList_.end());
}

void echoServer::onTimer()
{
    std::cout << "onTimer\n";
    Timestamp now = Timestamp::now();
    for (auto &it : connectionList_)
    {
        TcpConnectionPtr conn = it.conn.lock();
        if (conn)
        {
            double restm = now.microSecondsSinceEpoch() - it.lastReceiveTime.microSecondsSinceEpoch();
            restm /= Timestamp::kMicroSecondsPerSecond;
            if (restm > expiredTime)
            {
                conn->shutDown();
                server_.removeConnection(conn);
            }
            else if (restm < 0)
            {
                listNodeType * node = static_cast<listNodeType*>(conn->getAny());
                assert(node);
                node->lastReceiveTime = now;
            }
            else
            {
                break;
            }
        }
        else
        {
            connectionList_.erase(it.position);
        }
    }
}

int main()
{

    eventsLoop loop;
    IPAddr addr(8888);
    echoServer server(&loop , addr , 8);
    server.start();
    loop.loop();
}