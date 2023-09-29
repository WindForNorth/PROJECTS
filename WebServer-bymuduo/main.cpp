#include "WebServer.h"

int main(int argc , char * argv[])
{
    assert(argc == 3);
    int port = atoi(argv[1]);
    int threadnum = atoi(argv[2]);
    WebServer serv(port , threadnum);
    serv.init_sql_pool("localhost" , "root" , "520311yhf" , "mydb" , 3306 , 4 , false);
    serv.start();
}