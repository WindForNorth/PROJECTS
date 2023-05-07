#include "config.h"

int main(int argc, char *argv[])
{
    //需要修改的数据库信息,登录名,密码,库名
    string user = "root";
    string passwd = "520311yhf";
    string databasename = "mydb";

    //命令行解析
    //Config类里面配置了server运行需要的参数的默认值
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    //初始化
    //给server运行的必要参数赋值
    server.init(config.PORT, user, passwd, databasename, config.LOGWrite, 
                config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num, 
                config.close_log, config.actor_model);
    

    //日志
    server.log_write();//同步和模拟异步两种写日志的方式。同步使用当前线程写，模拟异步方式，采用队列存储需要写的日志，单独使用一个线程专门从队列取日志写（其实还是同步方式，不是真正意义上的异步）

    //数据库
    server.sql_pool();

    //线程池
    server.thread_pool();

    //触发模式
    server.trig_mode();

    //监听
    server.eventListen();

    //运行
    server.eventLoop();

    return 0;
}
