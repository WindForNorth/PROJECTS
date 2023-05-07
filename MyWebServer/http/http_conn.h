#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <fstream>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

//事务类，客户的请求被封装为一个类对象，通俗点说就是一个类对象就对应一条连接
class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 4096;
    static const int WRITE_BUFFER_SIZE = 4096;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    //主状态机可能的状态。状态机：根据包类型不同产生不同处理逻辑
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,//正在分析请求行
        CHECK_STATE_HEADER,//正在分析头部字段
        CHECK_STATE_CONTENT//
    };
    //处理http请求可能的结果
    enum HTTP_CODE
    {
        NO_REQUEST,//表示请求不完整，还需要等待客户的剩余信息
        GET_REQUEST,//表示获得一个完整请求
        BAD_REQUEST,//表示客户的请求存在语法问题
        NO_RESOURCE,//请求资源不存在
        FORBIDDEN_REQUEST,//表示客户无访问权限
        FILE_REQUEST,//表示用户消息是请求服务器文件
        INTERNAL_ERROR,//服务器内部错误
        CLOSED_CONNECTION,//表示客户已经关闭了连接
        FILE_UPLOAD//文件上传
    };
    //从状态机可能的状态
    enum LINE_STATUS
    {
        LINE_OK = 0,//表示已经完整读到一行
        LINE_BAD,//表示行出错
        LINE_OPEN//表示行数据尚不完整
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;
    int improv;


private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
    bool process_fileupload();

public:
    static int m_epollfd;//所有连接共享一个内核事件表
    static int m_user_count;
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];//读缓冲区
    int m_read_idx;//当前读缓冲区数据的尾后字节
    int m_checked_idx;//当前正在处理的字节
    int m_start_line;//行在缓冲区的起始位置
    char m_write_buf[WRITE_BUFFER_SIZE];//写缓冲区
    int m_write_idx;//写缓冲区中最后一个字节的尾后字节索引
    CHECK_STATE m_check_state;//记录主状态机的当前状态
    METHOD m_method;
    char m_real_file[FILENAME_LEN];//文件完整目录
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;//是否保持连接
    char *m_file_address;//共享内存起始地址
    struct stat m_file_stat;//用于在客户请求文件时，存储文件的属性，通过stat内置方法
    struct iovec m_iv[2];//需要发送的文件向量结构，通过向量结构中的指针记录发送缓冲区、通过另一个变量记录发送长度，可以调用writev方法一次性发送多个缓冲区的数据
    int m_iv_count;//指定上面向量结构待发送的个数
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;//需要发送的字节数
    int bytes_have_send;
    char *doc_root;
    char * username;//存储当前连接用户名，也可用于判断用户是否已经登录
    std::ofstream outfile;// 写文件流对象
    int userfile_bytesnum;//存储用户已经上传的字节数
    char * userfilename;
    int boundarysize;

    bool ifboundary;
    bool ifcontent_disposition;
    bool ifcontent_type;

    map<string, string> m_users;//存储数据库中已注册用户
    int m_TRIGMode;
    int m_close_log;

    //数据库用户名、密码、数据库名
    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
