#include <mysql/mysql.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <vector>
#include <time.h>
#include <sys/time.h>
// #include "../mysql/mysql_conn_pool.h"
// #include "../mysql/mysql_conn.h"
using std::vector;
int main()
{
    // MYSQL *CONN = nullptr;
    // const std::string username = "root";
    // const std::string passwd = "520311yhf";
    // const std::string db = "mydb";
    // const int port = 3306;
    // const std::string host = "localhost";
    // struct MYSQL_RES *_RES_ = nullptr;
    // if ((CONN = mysql_init(CONN)) == nullptr)
    // {
    //     printf("err init\n");
    //     exit(0);
    // }
    // if ((CONN = mysql_real_connect(CONN, host.data(), username.data(), passwd.data(), db.data(), port, nullptr, 0)) == nullptr)
    // {
    //     printf("err connect\n");
    //     exit(0);
    // }
    // if(mysql_query(CONN , "select username from user where username = 'hhh';") != 0)
    // {
    //     printf("err SQL\n");
    // }
    // else
    // {
    //     printf("success\n");
    // }
    // _RES_ = mysql_store_result(CONN);
    // if(mysql_query(CONN , "insert into user(username , passwd) values('test5' , 123456)") != 0)
    // {
    //     printf("err excute\n");
    //     exit(0);
    // }
    // if(mysql_query(CONN , "insert into user(username , passwd) values('test6' , 123456)") != 0)
    // {
    //     printf("err excute\n");
    //     exit(0);
    // }
    // if(mysql_query(CONN , "select username from user where username = 'sanyuan';") != 0)
    // {
    //     printf("err SQL\n");
    // }
    // else
    // {
    //     printf("success\n");
    // }
    // // if(mysql_query(CONN , "update user set username = 'username' where username = 'name';") != 0)
    // // {
    // //     printf("err excute\n");
    // //     exit(0);
    // // }
    // // if(mysql_query(CONN , "delete from user where username = 'user';") != 0)
    // // {
    // //     printf("err excute\n");
    // //     exit(0);
    // // }
    // // if ((_RES_ = mysql_store_result(CONN)) == nullptr)
    // // {
    // //     printf("err store result\n");
    // // }
    // // while (MYSQL_ROW ROW = mysql_fetch_row(_RES_))
    // // {
    // //     printf("username:%s pwd:%s\n", ROW[0], ROW[1]);
    // // }
    // // int column = mysql_num_fields(_RES_);
    // // int rows = mysql_num_rows(_RES_);
    // // printf("column:%d row:%d" , column , rows);

    // static std::string filename = "base";
    // static int size = 1024 * 1024;
    // static double flush = 2.0;

    MYSQL * conn = nullptr;
    mysql_close(conn);
    return 0;
}