#ifndef MYSQL_CONN_POOL_H
#define MYSQL_CONN_POOL_H
#include <mysql/mysql.h>
#include <semaphore.h>
#include <vector>
#include <string>
#include "mysql_conn.h"
#include "../../muduo/base/mutex.h"

class mysql_conn_pool
{
private:
    //数据库服务器的主机名
    std::string _host_;
    //连接用户名
    std::string _username_;
    //密码
    std::string _pwd_;
    //目的数据库名
    std::string _dbname_;
    //端口
    int _port_;

    bool _close_log_;

    int _sql_num_;
    //使用信号量同步连接数量
    sem_t _sem_;
    mutexLock _mutex_;
    std::vector<mysql_conn> _sql_pool_;
    std::vector<mysql_conn *>_sql_pool_pointor_;
public:
    mysql_conn_pool();
    ~mysql_conn_pool();
    void start();
    void init(const std::string &host , 
        const std::string &username , 
        const std::string &pwd , 
        const std::string &dbname ,
        int port,
        int sql_num,
        bool close_log);
    mysql_conn * getFreeSqlConn();
    void reconnect(mysql_conn *);
    void releseSqlConn(mysql_conn * conn);
};




#endif