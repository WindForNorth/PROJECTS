#include "Buffer.h"
#include <algorithm>
#include <iostream>
#include <sys/uio.h>
void Buffer::append(const char * st , size_t len)
{
    ensureSpace(len);
    std::copy(st , st + len , begin() + writerIndex_);
    writerIndex_ += len;
}

void Buffer::ensureSpace(size_t len)
{
    if(writeableBytes() < len){
        makeSpace(len);
    }
    assert(writeableBytes() >= len);
}

void Buffer::makeSpace(size_t len)
{
    if(writeableBytes() + preReverseSize() < preReverseBytes + len){
        //此时readIndex_前面的可写空间加上writerIndex_后面的可写空间不足len个字节
        buffer_.resize(writerIndex_ + len); //resize为添加之后的大小
    } else {
        //表示此时预留字节大小到可读字节之间的大小加上剩余可写大小足够容纳len字节内容
        //预留的8字节不能动
        //把readIndex_后面的内容拷贝到从初始位置开始
        assert(preReverseBytes < readerIndex_);
        size_t readn = readableBytes();
        std::copy(begin() + readerIndex_ , begin() + writerIndex_ , begin() + preReverseBytes);
        readerIndex_ = preReverseBytes;
        writerIndex_ = readerIndex_ + readn;
    }
}

size_t Buffer::readfd(int fd , int & error)
{
    //采用集中写
    //使用栈上的缓冲区作为预留读空间，足够大减少read的次数，即使一次无法读取完毕，水平触发方式也保证不会丢失数据
    char buff[65536];                   //64KB
    size_t writen = writeableBytes();
    struct iovec vec[2];                //sys/uio.h下
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len  = writen;
    vec[1].iov_base = buff;
    vec[1].iov_len  = sizeof buff;
    ssize_t n = readv(fd , vec , 2);     //sys/uio.h下
    if(n < 0){
        error = errno;
    } else if(n <= writen){
        //自带缓冲区就能装完
        writerIndex_ += n;
    } else {
        //使用到了额外空间，于是就得append
        //这里其实没有考虑需要读取的数据确实大于writen + sizeof buff情况，但是也不会丢失数据
        //如果需要改进，则判断在n == writen + sizeof buff的时候再读取一次
        writerIndex_ = buffer_.size();
        append(buff , n - writen);
    }
    return n;
}