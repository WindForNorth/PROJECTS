#include "mysql_conn_pool.h"
mysql_conn_pool::mysql_conn_pool() :
    _mutex_()
{
    
}

mysql_conn_pool::~mysql_conn_pool()
{
    // for(auto it : _sql_pool_)
    //     delete it;
}

void mysql_conn_pool::init(
        const std::string &host , 
        const std::string &username , 
        const std::string &pwd , 
        const std::string &dbname ,
        int port,
        int sql_num,
        bool close_log)
{
    _host_ = host;
    _username_ = username;
    _pwd_ = pwd;
    _dbname_ = dbname;
    _port_ = port;
    _sql_num_ = sql_num;
    _close_log_ = close_log;

    sem_init(&_sem_ , 0 , sql_num);
    _sql_pool_.reserve(sql_num);
    _sql_pool_pointor_.reserve(sql_num);
}

void mysql_conn_pool::start()
{
    for(int i = 0; i < _sql_num_; ++i)
    {
        MYSQL * CONN = nullptr;
        if((CONN = mysql_init(CONN)) == nullptr)
        {
            exit(0);
        }
        if((CONN = mysql_real_connect(CONN , 
            _host_.data() , _username_.data() , _pwd_.data() , _dbname_.data() , _port_ , nullptr , 0)) == nullptr)
        {
            exit(0);
        }
        _sql_pool_.emplace_back(CONN);
        _sql_pool_pointor_.push_back(&_sql_pool_.back());
    }
}

mysql_conn * mysql_conn_pool::getFreeSqlConn()
{
    sem_wait(&_sem_);
    mutexLockGuard lock(_mutex_);
    mysql_conn * ret = _sql_pool_pointor_.back();
    _sql_pool_pointor_.pop_back();
    return ret;
}

void mysql_conn_pool::releseSqlConn(mysql_conn * conn)
{
    mutexLockGuard lock(_mutex_);
    _sql_pool_pointor_.push_back(conn);
    sem_post(&_sem_);
}

void mysql_conn_pool::reconnect(mysql_conn * conn)
{
    MYSQL * _conn_ = conn->getConn();
    mysql_close(_conn_);
    if(!(_conn_ = mysql_init(_conn_)))
    {
        LOG("reinit error\n");
        return;
    }
    if(!(_conn_ = mysql_real_connect(_conn_ , _host_.data() , 
        _username_.data() , _pwd_.data() , _dbname_.data() , _port_ , nullptr , 0)))
    {
        LOG("reconnect error\n");
        return;
    }
    conn->setConn(_conn_);
}