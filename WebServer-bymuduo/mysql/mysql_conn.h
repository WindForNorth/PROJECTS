#ifndef MYSQL_CONN_H
#define MYSQL_CONN_H
#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <assert.h>
#include "../log.h"

extern AsyncLogging * log;

/*C库使用mysql的基本方式：
mysql_init(MYSQL * mysql);  //初始化mysql句柄
mysql_real_connect();   //发起连接
mysql_query();  //执行语句，增删改查都是一个接口，十分便利
mysql_store_result();   //存储结果集，是一个特定的结构，MYSQL_RES类型，但是用户无权拥有这个结构，只能使用指针取得读取权限
mysql_field_count();    //返回上一次执行语句想要查找的列数（注意不是结果的列数）
mysql_num_fields();     //返回指定结果集中的列
mysql_num_rows();       //返回指定结果集中的行数
mysql_fetch_row();      //从一个空行开始获取下一行结果
mysql_free_result();    //释放结果集

另外编译程序的时候需要连接mysql客户端的库 -lmysqlclient
*/


class mysql_conn
{
private:
    MYSQL * _conn_;
    //指向最近的一次结果集合
    struct MYSQL_RES * _RES_;
    void free_result() { mysql_free_result(_RES_); _RES_ = nullptr; }
    
public:
    mysql_conn(MYSQL * conn) :
        _conn_(conn),
        _RES_(NULL)
    {
        assert(_conn_ != nullptr);
    }
    ~mysql_conn()
    {
        if(_conn_) mysql_close(_conn_);
    }

    int addRecord(const std::string & q);
    int delRecord(const std::string & q);
    int updateRecord(const std::string & q);
    int seleRecord(const std::string & q);
    void getresult(std::vector<std::vector<std::string>> & res);
    void clearResult();
    bool isempty();
    void setConn(MYSQL * newconn) { _conn_ = newconn; }
    MYSQL * getConn() { return _conn_; }
};




#endif