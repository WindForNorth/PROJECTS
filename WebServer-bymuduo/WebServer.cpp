#include "WebServer.h"
#include "../muduo/net/Buffer.h"
#include "../muduo/net/TcpConnection.h"
#include "httprequest/httpRequestHandler.h"
#include "httprequest/responseType.h"


// 全局的日志变量
AsyncLogging * log = nullptr;

WebServer::WebServer(uint16_t port , const std::string & ip , int threadNum , bool useEpoll , bool closeLog , int sqlNum) : 
    _localAddr_(ip , port),
    _loop_(!useEpoll),
    _server_(&_loop_ , _localAddr_ , "WebServer" ,threadNum),
    _close_log_(closeLog),
    _alog_("WebServerLog" , LOGFILESIZE , FLUSHINTER),
    _sql_pool_()
{
    _server_.setConnectionCallBack(std::bind(&WebServer::handNewConn , this , std::placeholders::_1));
    _server_.setMessageCallBack(std::bind(&WebServer::handRead , this , 
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3
    ));
    log = &_alog_;
    _alog_.start();
}

WebServer::WebServer(uint16_t port , int threadNum , bool useEpoll , bool closeLog , int sqlNum
    ) :
    _localAddr_(port),
    _loop_(!useEpoll),
    _server_(&_loop_ , _localAddr_ , "WebServer" ,threadNum),
    _close_log_(closeLog),
    _alog_("WebServerLog" , LOGFILESIZE , FLUSHINTER),
    _sql_pool_()
{
    _server_.setConnectionCallBack(std::bind(&WebServer::handNewConn , this , std::placeholders::_1));
    _server_.setMessageCallBack(std::bind(&WebServer::handRead , this , 
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3
    ));
    _server_.setWriteComPleteCallBack(std::bind(&WebServer::handWrite , this , std::placeholders::_1));
    log = &_alog_;
    _alog_.start();
}

void WebServer::start()
{
    _server_.start();
    _sql_pool_.start();
    LOG("WebServer start up!\n");
    _loop_.loop();
}

void WebServer::handNewConn(const TcpConnectionPtr & conn)
{
    if(conn->isConnected())
    {
        //为新连接分配一个handler
        //handler负责请求处理状态维护
        httpRequestHandler * newHandler = new httpRequestHandler(conn , &_sql_pool_ , _close_log_);
        conn->setAny(static_cast<void *>(newHandler));
        LOG("a new conn from:" + conn->peerAddr().toHostPort() + "\n");
    }
    else
    {
        LOG("disconnect\n");
        httpRequestHandler * tmp = static_cast<httpRequestHandler *>(conn->getAny());
        if(tmp != nullptr)
        {
            delete tmp;
        }
    }
}

void WebServer::handRead(const TcpConnectionPtr & conn , Buffer * buff , Timestamp stamp)
{
    httpRequestHandler * tmp = static_cast<httpRequestHandler *>(conn->getAny());
    if(tmp != nullptr)
    {
        tmp->handRequest(buff);
    }
    else
    {
        std::string res = http_version + " 500 " + server_err_500type + "\r\n";
        res += "Connection:close\r\n\r\n";
        conn->send(res);
        LOG("an except happend\n");
        _server_.removeConnection(conn);
    }
}

//写完成回调，检查用户是否还有数据需要发送，如果有，加入缓冲区
void WebServer::handWrite(const TcpConnectionPtr & conn)
{
    httpRequestHandler * tmp = static_cast<httpRequestHandler *>(conn->getAny());
    if(tmp != nullptr)
    {
        tmp->continueSend();
    }
}
WebServer::~WebServer()
{

}
