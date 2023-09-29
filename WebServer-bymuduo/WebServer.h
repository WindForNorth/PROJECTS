#ifndef WEBSERVER_H
#define WEBSERVER_H
#include "../muduo/net/TcpServer.h"
#include "../muduo/net/eventsLoop.h"
#include "../muduo/base/AsyncLogging.h"
#include "../muduo/net/IPAddr.h"
#include "../muduo/net/Timestamp.h"
#include "log.h"
#include "mysql/mysql_conn_pool.h"

const int LOGTHREAD = 1;
const int LOGFILESIZE = 1024 * 1024 * 512;  //512MiB
const double FLUSHINTER = 2.0;
const double EXPIRE_TO_CLOSE = 10.0;      //超时断连
class Buffer;

class WebServer
{
private:
    // typedef ConnectionCallBackType ConnectionCallBackType;
    // typedef MessageCallBackType MessageCallBackType;
    // typedef TcpConnectionPtr TcpConnectionPtr;
private:
    IPAddr _localAddr_;
    eventsLoop _loop_;
    TcpServer _server_;
    AsyncLogging _alog_;
    
    bool _close_log_;
    //数据库连接池
    mysql_conn_pool _sql_pool_;
public:
    WebServer(uint16_t port , 
            const std::string & ip , 
            int threadNum  = 4, 
            bool useEpoll = true , 
            bool closeLog = false , 
            int sqlNum = 4);
    WebServer(uint16_t port , int threadNum , bool useEpoll = true , bool closeLog = false , int sqlNum = 4);
    void init_sql_pool(const std::string &host , 
        const std::string &username , 
        const std::string &pwd , 
        const std::string &dbname ,
        int port,
        int sql_num,
        bool close_log) { _sql_pool_.init(host , username , pwd , dbname , port , sql_num , close_log); }
    void start();
    void handRead(const TcpConnectionPtr & conn , Buffer * buff , Timestamp stamp);
    void handWrite(const TcpConnectionPtr & conn);
    void handNewConn(const TcpConnectionPtr & conn);
    
    ~WebServer();
};


#endif