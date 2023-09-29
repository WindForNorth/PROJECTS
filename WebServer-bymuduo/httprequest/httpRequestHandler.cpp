#include "httpRequestHandler.h"
#include <string.h>
#include <cstring>
#include <sys/stat.h>
#include "responseType.h"


extern AsyncLogging * log;
/*几个C/C++库的字符串处理函数，挺好用的

//向dest开始拷贝src
char *strcpy (char *__restrict __dest, const char *__restrict __src)

//将s指向的缓冲区设置为c，最大设置n个
void *memset (void *__s, int __c, size_t __n)

//将s指向的缓冲区全部清0，其实就是上面那个函数设置为\0
void bzero (void *__s, size_t __n)

//这个是c++的，头文件是cstring
//相当于模式匹配算法，从s1中匹配s2中任意字符第一次出现的位置，返回那个位置的指针
char* strpbrk(char* __s1, const char* __s2)

//忽略大小写比较s1和s2是否相等，相等时返回0
int strcasecmp(const char *__s1, const char *__s2)

//从s开始，返回连续在accept中出现的字符数量
size_t strspn(const char *__s, const char *__accept)

//类似与strpbrk函数，该函数返回s中c第一次出现的位置，但是c只能是一个整型（char也是）
char *strchr(char *__s, int __c)

//模式匹配算法，返回第二个参数指定的字符串在第一个参数中的起始位置
const char *strstr (const char *__haystack, const char *__needle)
*/

httpRequestHandler::httpRequestHandler(
    const TcpConnectionPtr &conn, 
    mysql_conn_pool *sql_pool, bool close_log) : 

    _close_log_(close_log),
    _sql_conn_pool_(sql_pool),
    _baseSourceName_("/home/monster/data/WebSource"),
    _fstream_(nullptr),
    _st_idx_(0),
    _ed_idx_(0),
    _conn_(conn)
{
    resetStatus();
    _req_content_.eof = 0;
    _req_content_.s_pos = 0;
    _req_content_.filesize = 0;
    _rest_File_Size_ = 0;
    _req_content_.range = false;
}

httpRequestHandler::~httpRequestHandler()
{
    if(_fstream_)
    {
        fclose(_fstream_);
    }
}

void httpRequestHandler::resetStatus()
{
    _read_status_ = NO_LINE;
    _hand_status_ = HANDING_COM;
    _hand_result_ = NO_REQUEST;
    _method_ = NO_METHOD;
    _rest_File_Size_ = 0;
    _req_content_.contentlen = 0;
    _req_content_.filesize = 0;
    _req_content_.s_pos = 0;
    _req_content_.range = false;
    _req_content_.eof = 0;

    if(_fstream_)
    {
        fclose(_fstream_);
        _fstream_ = nullptr;
    }
}

void httpRequestHandler::continueSend()
{
    
    // 注意这里可能出现一种情况：本次回调可能原本应该处理上一次的请求
    // 但是由于用户中途发起了新的请求，导致这个函数变成了处理这个请求
    // 但是这并不会出错，只要用户的状态结构_req_content_正确
    // 换言之，这个结构永远记录最新的请求
    if (_fstream_ != nullptr && _rest_File_Size_ > 0)
    {
        // _buff_通常是没有必要的
        char buff[MAXSTACKBUFF_SIZE];
        size_t rest_buff_bytes = _ed_idx_ - _st_idx_;
        if(rest_buff_bytes)
        { 
            strncpy(buff, _buff_ + _st_idx_, rest_buff_bytes);
            _ed_idx_ = 0;
            _st_idx_ = 0;
        }
        size_t writeablenytes = 
            MAXSTACKBUFF_SIZE - rest_buff_bytes > _rest_File_Size_ ?
                _rest_File_Size_ : MAXSTACKBUFF_SIZE - rest_buff_bytes;
        
        size_t bytes = fread(buff , 1 , writeablenytes , _fstream_);
        _req_content_.s_pos += bytes;
        _rest_File_Size_ -= bytes;
        _conn_->send(buff, rest_buff_bytes + bytes);
        if (_rest_File_Size_ < 0)
        {
            LOG("write data error\n");
            _rest_File_Size_ = 0;
        }
    }
    else
    {
        // 这表示用户的一次请求彻底完成
        // 重置状态
        if(_rest_File_Size_ <= 0)
        {
            LOG("filesize zero\n");
        }
        if(_fstream_ == nullptr)
        {
            LOG("empty stream\n");
        }
        resetStatus();
    }
}

void httpRequestHandler::handRequest(Buffer *buff)
{
    // 此处可考虑是否使用while持续处理请求
    size_t num_of_line = 0;
    if(_hand_status_ == HANDING_BODY)
    {
        // 如果同一个用户的上次请求未完成
        // 则不继续处理新的请求，而是接着发送上次未发送完成的数据
        continueSend();
        return;
    }
    num_of_line = readLine(buff);
    if (_read_status_ == LINE_BAD)
    {
        _hand_result_ = BAD_REQUEST;
    }
    else if (num_of_line > 0)
    {
        handLine(buff, num_of_line);
    }
    if (_hand_result_ != NO_REQUEST)
    {
        char stack_buff[OUTPUTBUFF_SIZE];
        size_t bytes = addResponse(stack_buff, OUTPUTBUFF_SIZE);
        _conn_->send(stack_buff, bytes);
        return;
    }
}

size_t httpRequestHandler::readLine(void *buff)
{
    size_t num_of_line = 0;
    Buffer *buff_ = static_cast<Buffer *>(buff);
    const char *s = buff_->peek();        // 第一个可读字符
    const char *ed = buff_->beginWrite(); // 尾后字符
    while (s != ed)
    {
        if (*s == '\r')
        {
            if (s == ed - 1)
            {
                _read_status_ = WAIT_MORE;
            }
            else if (*(s + 1) == '\n')
            {
                ++num_of_line;
                _read_status_ = NO_LINE;
                LOG("get a complete request\n");
            }
            else
            {
                _read_status_ = LINE_BAD;
            }
            return num_of_line;
        }
        const char *p = strchr(s, '\n');
        if (p != nullptr && p != s)
        {
            char c = *(p - 1);
            if (c == '\r')
            {
                ++num_of_line;
            }
            else
            {
                _read_status_ = LINE_BAD;
                return 0;
            }
        }
        else if (p == nullptr)
        {
            _read_status_ = WAIT_MORE;
            return num_of_line;
        }
        else
        {
            _read_status_ = LINE_BAD;
            return 0;
        }
        s = p + 1;
    }
    _read_status_ = WAIT_MORE;
    return num_of_line;
}

void httpRequestHandler::handLine(void *buff, size_t len)
{

    for (int i = 0; i < len; ++i)
    {
        switch (_hand_status_)
        {
        case HANDING_COM:
        HANDING_REQLINE:
        {
            handreqline(buff);
            if (_hand_result_ == BAD_REQUEST)
                return;
            break;
        }

        case HANDING_HEADER:
        {
            handheader(buff);
            break;
        }
        default:
            break;
        }
    }
    handbody(buff);
}


size_t httpRequestHandler::addResponse(void *buff, size_t len)
{
    char * bf = static_cast<char *>(buff);
    size_t addbytes = 0;
    std::string bad_msg("");
    switch (_hand_result_)
    {
    case GET_REQUEST:
    {
        LOG("static file serv\n");
        addbytes = serv_staticFile(buff , len);
        break;
    }
    case BAD_REQUEST:
    {
        addbytes += addHttpStatus("400" , client_err_400type , buff);
        bad_msg = (std::move(clientError("400", client_err_400type, client_err_400longtype)));
        break;
    }
    case INTERNAL_ERROR:
    {
        addbytes += addHttpStatus("500" , server_err_500type , buff);
        bad_msg = (std::move(clientError("500", server_err_500type, server_err_500longtype)));
        break;
    }
    case NOT_FOUND:
    {
        addbytes += addHttpStatus("404" , client_err_404type , buff);
        bad_msg = (std::move(clientError("404", client_err_404type, client_err_404longtype)));
        break;
    }
    case FORBIDDEN:
    {
        addbytes += addHttpStatus("403" , client_err_403type , buff);
        bad_msg = (std::move(clientError("403", client_err_403type, client_err_403longtype)));
        break;
    }
    case TIMEOUT:
    {
        addbytes += addHttpStatus("408" , client_err_408type , buff);
        bad_msg = (std::move(clientError("408", client_err_408type, client_err_408longtype)));
        break;
    }
    case NO_REQUEST:
    {
        break;
    }
    default:
        break;
    }
    if(!bad_msg.empty())
    {
        
        addbytes += addContent_Length(bf + addbytes , len - addbytes , bad_msg.size());
        addbytes += addHttpGenHeader("text/html" , bf + addbytes);
        addbytes += addErrorMsg(bf + addbytes , bad_msg , sizeof(bf) - addbytes);
    }
    return addbytes;
}

size_t httpRequestHandler::readSoure(void *buff, size_t len)
{

}

void httpRequestHandler::handreqline(void *buff)
{
    Buffer *tmp = static_cast<Buffer *>(buff);
    const char *s = tmp->peek();
    const char *p = strchr(s, ' ');
    std::string str(s, p);
    if (strcasecmp(str.data(), "GET") == 0)
        _method_ = GET;
    else if (strcasecmp(str.data(), "POST") == 0)
        _method_ = POST;
    else if (strcasecmp(str.data(), "HEAD") == 0)
        _method_ = HEAD;
    else
    {
        // 客户请求有误
        LOG("client msg:\n" + tmp->retrieveTostring() + "\n");
        _hand_result_ = BAD_REQUEST;
        return;
    }
    p += strspn(p, " \t");
    s = p;
    p = strchr(s, ' ');
    _req_content_.url = std::move(std::string(s, p));
    // _hand_result_ = GET_REQUEST;
    LOG("get url:" + _req_content_.url + "\n");
    _hand_status_ = HANDING_HEADER;
    s = tmp->peek();
    size_t len = strchr(s, '\n') - s + 1;
    tmp->retrieve(len);
}

void httpRequestHandler::handheader(void *buff)
{
    Buffer *tmp = static_cast<Buffer *>(buff);
    const char *s = tmp->peek();
    const char *ed = strchr(s, '\r');
    if (*s == '\r')
    {
        tmp->retrieve(2);
        _hand_status_ = HANDING_BODY;
        return;
    }
    if (strncasecmp(s, "Cookie:", 7) == 0)
    {
        s += 7;
        s += strspn(s, " \t");
        _req_content_.cookie = std::move(std::string(s, ed));
    }
    else if (strncasecmp(s, "Content-length:", 15) == 0)
    {
        s += 15;
        s += strspn(s, " \t");
        std::string str(s, ed);
        _req_content_.contentlen = atol(str.data());
    }
    else if(strncasecmp(s, "Range:", 6) == 0)
    {
        _req_content_.range = true;
        s = strchr(s , '=');
        if(s)
        {
            const char * tmp = strchr(s , '-');
            if(tmp)
            {
                // 这里是没有关注结尾长度的
                // 假定没有范围传输
                std::string len(s + 1 , tmp);
                _req_content_.s_pos = static_cast<off_t>(atol(len.data()));
                if(tmp + 1 != ed)
                {
                    std::string eof(tmp + 1 , ed);
                    _req_content_.eof = static_cast<int64_t>(atol(eof.data()));
                }
            }
        }
    }
    else
    {
        std::string logmsg = "unkonw segment:" + std::string(tmp->peek(), ed);
        LOG(logmsg + "\n");
    }
    s = tmp->peek();
    size_t len = ed - s + 2;
    tmp->retrieve(len);
}

void httpRequestHandler::handbody(void *buff)
{
    std::string url = _req_content_.url;
    if (check_logstatus() || url == "/register.html" || url == "/log.html")
    {
        if (url == "/register" || url == "/cgi")
        {
            LOG("check Account\n");
            checkAccount(buff);
        }
        else if (check_logstatus() && (url == "/" || url == "/log.html"))
        {
            _req_content_.url = "/welcome.html";
        }
        _hand_result_ = GET_REQUEST;
        return;
    }
    else
    {
        if (url == "/register" || url == "/cgi")
        {
            LOG("check Account\n");
            checkAccount(buff);
            
        }
        else
        {
            _req_content_.url = "/judge.html";
            _hand_result_ = GET_REQUEST;
        }
    }
}

// 超时未获取完整请求
void httpRequestHandler::timeout()
{
}

void httpRequestHandler::checkAccount(void * buff)
{
    std::string username("");
    std::string passwd("");
    
    getAccount(username, passwd , buff);
    if (username.empty() || passwd.empty())
    {
        std::string url = _req_content_.url;
        if (url == "/register")
        {
            _req_content_.url = "/registerError.html";
        }
        else
        {
            _req_content_.url = "/logError.html";
        }
        _hand_result_ = GET_REQUEST;
        return;
    }
    else
    {
        LOG("mysql check start\n");
        mysql_conn *sql_conn = _sql_conn_pool_->getFreeSqlConn();
        assert(sql_conn);
        int ret = -1;
        if (_req_content_.url == "/register")
        {
            std::string q ("select username from user where username = '" + 
                username + "';");
            // 考虑是否加入重试的次数
            // while可能不太合理
            while((ret = sql_conn->seleRecord(q)) != 0)
            {
                // mysql默认8小时后断开连接，此时需要重新建立连接
                LOG("reconnect...\n");
                _sql_conn_pool_->reconnect(sql_conn);
            }
            if (sql_conn->isempty())
            {
                // 注意，select查询执行之后，必须要取走结果集才能继续执行SQL语句
                // 也就是要mysql_store_result调用
                // 但是其他非查询类语句不影响
                if(sql_conn->addRecord("insert into user(username , passwd) values('" + 
                                            username + "' , '" + passwd + "');") != 0)
                {
                    _hand_result_ = INTERNAL_ERROR;
                    LOG("SQL add error\n");
                }
                else
                {
                    _hand_result_ = GET_REQUEST;
                    _req_content_.url = "/welcome.html";
                }
            }
            else
            {
                _req_content_.url = "/registerError.html";
                _hand_result_ = GET_REQUEST;
            }
            _sql_conn_pool_->releseSqlConn(sql_conn);
        }
        else if (_req_content_.url == "/cgi")
        {
            std::string q("select username , passwd from user where username = '" + 
                                    username + "' and passwd = '" + passwd + "';");
            while((ret = sql_conn->seleRecord(q)) != 0)
            {
                LOG("reconnect...\n");
                _sql_conn_pool_->reconnect(sql_conn);
            }
            if (sql_conn->isempty())
            {
                _req_content_.url = "/logError.html";
            }
            else
            {
                _req_content_.url = "/welcome.html";
            }
            _sql_conn_pool_->releseSqlConn(sql_conn);
            _hand_result_ = GET_REQUEST;
        }
    }
}

void httpRequestHandler::getAccount(std::string &username, std::string &passwd , void * buff)
{
    Buffer *tmp = static_cast<Buffer *>(buff);
    if (tmp->readableBytes() >= _req_content_.contentlen)
    {
        const char *st = strchr(tmp->peek(), '=');
        const char *ed = strchr(tmp->peek(), '&');
        if (st && ed)
        {
            
            username = std::string(st + 1, ed);
        }
        if(ed) st = strchr(ed, '=');
        if (st && ed)
        {
            passwd = std::string(st + 1, _req_content_.contentlen - (st - tmp->peek() + 1));
        }
        if(ed) tmp->retrieve(_req_content_.contentlen);
    }
}

const std::string httpRequestHandler::clientError(
    const std::string &errnum,
    const std::string &shortmsg,
    const std::string &longmsg)
{
    std::string header = "<html><title>Request Error</title>";
    std::string body = "<body bgcolor = "
                       "oxffffff"
                       ">\r\n";
    std::string content = errnum + ":" + shortmsg + "\r\n<p>" + longmsg + "</p>\r\n" + "</body>\r\n</html>";
    return header + body + content;
}

size_t httpRequestHandler::addErrorMsg(void *buff, std::string &bad_msg , size_t len)
{
    size_t addbytes = bad_msg.size();
    if (len < addbytes)
    {
        strncpy(static_cast<char *>(buff), bad_msg.data(), len);
        strcpy(_buff_, &(*(bad_msg.begin() + len)));
        _rest_File_Size_ = bad_msg.size() - len;
        _ed_idx_ = _rest_File_Size_;
        addbytes = len;
    }
    else
        strcpy(static_cast<char *>(buff), bad_msg.data());

    return addbytes;
}

size_t httpRequestHandler::addHttpGenHeader(const std::string & filetype , void * buff)
{
    size_t ret = 0;
    auto bf = static_cast<char *>(buff);
    char tmp_buf[100];
    gethostname(tmp_buf , sizeof tmp_buf);
    ret += sprintf(bf + ret , "Server:%s\r\n" , tmp_buf);
    ret += sprintf(bf + ret , "Connection:%s\r\n" , "keep-alive");
    // cookie的设置时机：用户身份已经得到验证并且旧有cookie已经过期
    if (_req_content_.url == "/welcome.html" && _req_content_.cookie.empty())
        ret += sprintf(bf + ret , "Set-Cookie:log_disaster=%s;Max-Age=%d\r\n" , "true" , MAX_AGE);
    ret += sprintf(bf + ret , "Content-type:%s\r\n\r\n" , filetype.data());
    return ret;
}

const std::string httpRequestHandler::getFileType()
{
    std::string filetype = "";
    std::string url = _req_content_.url;
    if(url != "")
    {
        if(strstr(url.data() , ".html"))
        {
            filetype = "text/html";
        }
        else if (strstr(url.data() , ".txt"))
        {
            filetype = "text/plain";
        }
        else if (strstr(url.data() , ".jpg"))
        {
            filetype = "image/jpg";
        }
        else if (strstr(url.data() , ".png"))
        {
            filetype = "image/png";
        }
        else if (strstr(url.data() , ".jpeg"))
        {
            filetype = "image/jpeg";
        }
        else if (strstr(url.data() , ".mp4"))
        {
            filetype = "video/mp4";
        }
        else if (strstr(url.data() , ".avi"))
        {
            filetype = "video/avi";
        }
    }
    return filetype;
}

size_t httpRequestHandler::serv_staticFile(void * buff , size_t len)
{
    char * bf = static_cast<char *>(buff);
    std::string url = _req_content_.url;
    std::string completeFilename(_baseSourceName_);
    if(url == "/")
    {
        completeFilename += "/judge.html";
    }
    else
    {
        completeFilename += url;
    }
    struct stat statbf;
    LOG("filename:" + completeFilename + "\n");
    if(stat(completeFilename.data() , &statbf) == -1)
    {
        std::string bad_msg(std::move(clientError("404" , client_err_400type , client_err_400longtype)));
        LOG("not found\n");
        size_t bytes = addHttpStatus("404" , client_err_404type , bf);
        bytes += addContent_Length(bf + bytes, len - bytes , bad_msg.size());
        bytes += addHttpGenHeader("text/html" , bf + bytes);
        return bytes + addErrorMsg(bf + bytes, bad_msg , len - bytes);
    }
    if(!S_ISREG(statbf.st_mode) || !(statbf.st_mode & (S_IRUSR | S_IRGRP)))
    {
        std::string bad_msg(std::move(clientError("403" , client_err_403type , client_err_403longtype)));
        LOG("permission not\n");
        size_t bytes = addHttpStatus("404" , client_err_404type , bf);
        bytes += addContent_Length(bf + bytes, len - bytes , bad_msg.size());
        bytes += addHttpGenHeader("text/html" , bf + bytes);
        return bytes + addErrorMsg(bf + bytes, bad_msg , len - bytes);
    }
    _fstream_ = fopen(completeFilename.data() , "r");
    if(_fstream_ == nullptr)
    {
        std::string bad_msg(std::move(clientError("500" , server_err_500type , server_err_500longtype)));
        LOG("Internal server error\n");
        size_t bytes = addHttpStatus("404" , client_err_404type , bf);
        bytes += addContent_Length(bf + bytes, len - bytes , bad_msg.size());
        bytes += addHttpGenHeader("text/html" , bf + bytes);
        return bytes + addErrorMsg(bf + bytes, bad_msg , len - bytes);
    }

    // 正常处理即将发送的文件
    size_t writenBytes = 0;
    size_t Content_Length = 0;
    _req_content_.filesize = static_cast<uint64_t>(statbf.st_size);
    if(_req_content_.eof != 0)
    {
        Content_Length = _req_content_.eof - _req_content_.s_pos + 1;
    }
    else
    {
        Content_Length = _req_content_.filesize - _req_content_.s_pos;
        _req_content_.eof = _req_content_.filesize - 1;
    }
    std::string filetype(std::move(getFileType()));
    fpos_t _pos;
    _pos.__pos = _req_content_.s_pos;
    // 定位流
    fsetpos(_fstream_ , &_pos);
    if(_req_content_.range)
    {
        // 此时处于非首次请求同一个文件
        writenBytes += addHttpStatus("206" , "Partial Content" , buff);
        writenBytes += addContent_Length(bf + writenBytes , len - writenBytes , Content_Length);
        writenBytes += addContent_Range(bf + writenBytes , len - writenBytes);
    }
    else
    {
        // 首次请求
        writenBytes += addHttpStatus("200" , "OK" , buff);
        writenBytes += sprintf(bf + writenBytes  , "Accept-Ranges:bytes\r\n");
        writenBytes += addContent_Length(bf + writenBytes , len - writenBytes , Content_Length);
    }
    writenBytes += addHttpGenHeader(filetype , bf + writenBytes);
    // 读取文件
    size_t bytes = fread(bf + writenBytes , 1 , len - writenBytes , _fstream_);
    _req_content_.s_pos += bytes;
    _rest_File_Size_ = _req_content_.filesize - _req_content_.s_pos;
    return writenBytes + bytes;
}

inline size_t httpRequestHandler::addHttpStatus(const std::string & staCode , 
    const std::string & shortmsg, void * buff)
{
    return sprintf(static_cast<char *>(buff) , "HTTP/1.1 %s %s\r\n" , staCode.data() , shortmsg.data());
}

inline size_t httpRequestHandler::addContent_Range(void * buff , size_t len)
{
    return sprintf(static_cast<char *>(buff) , 
        "Content-Range:bytes %ld-%ld/%ld\r\n" , 
            _req_content_.s_pos , _req_content_.eof , _req_content_.filesize);
}

inline size_t httpRequestHandler::addContent_Length(void *buff, size_t len, size_t filesize)
{
    return sprintf(static_cast<char *>(buff) , "Content-Length:%lu\r\n" , filesize);
}
inline size_t httpRequestHandler::Set_Cookie(void *buff, size_t len)
{
    return sprintf(static_cast<char *>(buff) , "Set-Cookie:log_disaster=true\r\n");
}