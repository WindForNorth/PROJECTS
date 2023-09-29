#ifndef HANDLE_HTTP_H
#define HANDLE_HTTP_H
#include "../log.h"
#include "../../muduo/net/TcpConnection.h"
#include "../../muduo/net/Buffer.h"
#include "../mysql/mysql_conn_pool.h"
#include <fcntl.h>
#include <unordered_set>

// 全局的set检查用户的登录状态
// std::unordered_set<std::string> log_set;


const int MAX_AGE = 60 * 24 * 60;    //cookie超时时长
const int OUTPUTBUFF_SIZE = 4096;
const int MAX_HEADER_BYTES = 1024;
const int MAXSTACKBUFF_SIZE = 8 * 1024;
//请求方式
typedef enum
{
    //只支持最基本的请求方式
    GET = 0, POST , HEAD , NO_METHOD
}REQUEST_METHOD;

//正在分析报文的哪个位置，请求行？首部行？消息体？
//是否真的需要还有待商榷
typedef enum
{
    HANDING_REQLINE = 4,HANDING_HEADER , HANDING_BODY , HANDING_COM
}HAND_STATUS;

//请求处理结果
typedef enum
{
    GET_REQUEST = 8, BAD_REQUEST , INTERNAL_ERROR , NOT_FOUND , FORBIDDEN , TIMEOUT , NO_REQUEST
}HAND_RESULT;

//请求消息的读取进度（以行为单位）
typedef enum
{
    LINE_OK = 15, LINE_BAD , WAIT_MORE , NO_LINE
}READ_REQMSG_STATUS;

typedef struct 
{
    std::string url;
    std::string cookie;
    // 当前文件流的起始位置
    uint64_t s_pos;
    // 存储客户端body长度
    uint64_t contentlen;
    // 存储请求文件的总大小
    uint64_t filesize;
    // 请求文件流结束位置
    uint64_t eof;
    bool range;
} req_msg;


class httpRequestHandler
{
private:
    bool _close_log_;
    mysql_conn_pool * _sql_conn_pool_;
    std::string _baseSourceName_;

    //文件状态有关
    FILE * _fstream_; //二进制流打开文件
    uint64_t _rest_File_Size_;
    char _buff_[OUTPUTBUFF_SIZE]; //尽量不使用，应该说完全用不上才对

    //未发送数据的起始位置
    int _st_idx_;
    int _ed_idx_;

    //状态机处理状态
    //随时处理随时更新
    HAND_RESULT _hand_result_;
    READ_REQMSG_STATUS  _read_status_;
    HAND_STATUS _hand_status_;
    REQUEST_METHOD _method_;

    //连接实体
    const TcpConnectionPtr _conn_;


    //请求相关内容
    req_msg _req_content_;

    //只负责读取请求和首部行
    //返回读取的完整行数
    size_t readLine(void * buff );
    
    //处理读取消息
    void handLine(void * buff , size_t len);

    //根据状态机处理状态返回消息
    //并最终更新状态（决定是否回到初始状态）
    size_t addResponse(void * buff , size_t len);

    //读取请求资源到缓冲区
    size_t readSoure(void * buff , size_t len);

    void resetStatus();
    
    void handreqline(void * buff);
    void handheader(void * buff);
    void handbody(void * buff);
    void timeout();
    void checkAccount(void * buff);
    void getAccount(std::string & username , std::string & passwd , void * buff);
    size_t addErrorMsg(void * buff , std::string & msg , size_t len);
    const std::string  clientError(
                    const std::string & errnum , 
                    const std::string & shortmsg , 
                    const std::string & longmsg);
    
    // 添加通用头部
    size_t addHttpGenHeader(const std::string & filetype , void * buff);
    inline size_t addHttpStatus(const std::string & staCode , const std::string & shortmsg , void *buff);
    inline size_t addContent_Range(void * buff , size_t len);
    inline size_t addContent_Length(void * buff , size_t len , size_t filesize);
    inline size_t Set_Cookie(void * buff , size_t len);
    const std::string getFileType();
    size_t serv_staticFile(void * buff , size_t len);
    bool check_logstatus() { return _req_content_.cookie.size() != 0; }
public:
    httpRequestHandler(const TcpConnectionPtr &conn , mysql_conn_pool * sql_pool , bool close_log = false);
    ~httpRequestHandler();
    void handRequest(Buffer * buff);
    void continueSend();
    bool isconnected() {return _conn_->isConnected();}
    const TcpConnectionPtr & getconn() { return _conn_; }
};




#endif