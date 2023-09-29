#include "Config.h"
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
void usage()
{
    std::cout << "Usage:\n";
    std::cout << "  [-p portnum] [-s sql_num] [-t thread_num] [-c <=0 will close log] "
              << "[-a opt 1 is Reactor] [-e opt 1 will use epoll]\n";
}
/*
getopt()的外部变量

opterr:正常情况下为0，发生错误时为1
optopt:发生错误时即为错误参数
optarg:错误参数值
optind:argv的当前索引值
*/
void Config::handArgs(int argc , char * argv[])
{
    int o;
    const char * opts = "p:s:t:c:a:e:";
    while(o = getopt(argc , argv , opts) != -1)
    {
        switch (o)
        {
        case 'p':
            port_ = atoi(optarg);
            break;
        case 's':
            sql_num_ = atoi(optarg);
            break;
        case 't':
            thread_num_ = atoi(optarg);
            break;
        case 'c':
            close_log_ = atoi(optarg);
            break;
        case 'a':
            actor_mode_ = atoi(optarg);
            break;
        case 'e':
            epoll_ = atoi(optarg);
            break;
        case '?':
            std::cout << "invalid argument: " << optopt << "\n";
            usage();
            break;
        default:
            break;
        }
    }    
}