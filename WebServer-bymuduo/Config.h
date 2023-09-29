#ifndef CONFIG_H
#define CONFIG_H

class Config
{
private:
    //端口
    int port_;
    
    //数据库连接池大小
    int sql_num_;

    //线程池大小
    int thread_num_;

    //是否关闭日志
    bool close_log_;

    //并发模型
    int actor_mode_;

    //IO复用方式
    int epoll_;
public:
    Config() : 
    port_(8888),
    sql_num_(4),
    thread_num_(4),
    close_log_(false),
    actor_mode_(1),
    epoll_(1)
    {}
    ~Config(){}

    void handArgs(int argc , char * argv[]);
};


#endif