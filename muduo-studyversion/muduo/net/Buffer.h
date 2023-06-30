#ifndef BUFFER_H
#define BUFFER_H
#include <vector>
#include <assert.h>
#include <string>
class Buffer
{
private:
    //buffer
    std::vector<char>buffer_;
    int readerIndex_;                           //缓冲区读起始下标
    int writerIndex_;                           //缓冲区写起始下标

    //private func
    char * begin() { return &*buffer_.begin(); }
    void ensureSpace(size_t len);
    void makeSpace(size_t len);
public:
    static const size_t preReverseBytes = 8;       //在缓冲区前面预留8字节大小
    static const size_t defaultInitBytes = 1024;   //
    Buffer(/* args */) : 
        readerIndex_(preReverseBytes),
        writerIndex_(preReverseBytes),
        buffer_(defaultInitBytes + preReverseBytes)
    {

    }
    ~Buffer() = default;

    size_t readfd(int fd , int & error);
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writeableBytes() const { return buffer_.size() - writerIndex_; }
    size_t preReverseSize() const { return readerIndex_; }                  //返回缓冲区前面可读大小
    const char * peek() { return begin() + readerIndex_; }                  //返回起始可读位置指针
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        readerIndex_ += len;                                //将可读位置后移相当于取走了len字节大小的内容
    }
    void retriveAll()
    {
        readerIndex_ = preReverseBytes;
        writerIndex_ = preReverseBytes; 
    }

    std::string retrieveTostring()
    {
        std::string ret(peek() , readableBytes());
        retriveAll();                                       //重置缓冲区
        return ret;
    }
    //在缓冲区末尾添加
    void append(std::string & str)
    {
        append(str.data() , str.length());
    }
    void append(const void * data , size_t len)
    {
        append(static_cast<const char *>(data) , len);
    }

    char * beginWrite() { return begin() + writerIndex_; }
    char * beginRead() { return begin() + readerIndex_; }
    void append(const char * st , size_t len);
};


#endif