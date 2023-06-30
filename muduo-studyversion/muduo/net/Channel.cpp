
#include "Channel.h"
#include <poll.h>
#include <sys/epoll.h>
#include <iostream>
#include "eventsLoop.h"
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

static_assert(POLLHUP == EPOLLHUP, "EPOLLHUP != POLLHUP");
static_assert(POLLIN == EPOLLIN, "EPOLLIN != POLLIN");
static_assert(POLLERR == EPOLLERR, "POLLERR != EPOLLERR");
static_assert(POLLRDHUP == EPOLLRDHUP, "POLLRDHUP != EPOLLRDHUP");
static_assert(POLLPRI == EPOLLPRI, "POLLPRI == EPOLLPRI");
static_assert(POLLOUT == EPOLLOUT, "POLLOUT == EPOLLOUT");

Channel::Channel(eventsLoop *loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1)
{
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp ReceiveTime)
{
    // 根据evens_的值分别处理不同的回调
    // epoll和poll的几个基本事件的定义是相同的
    // 所以可以同时支持
    eventHandling_ = true;
    if (revents_ & POLLHUP && !(revents_ & POLLIN))
    {
        // 用户已经挂起连接（应该指的是TIME_WAIT期间），并且已经没有可读数据
        std::cout << "client has closed the connection\n";
        if (closeCallBack_)
            closeCallBack_();
    }
    if (revents_ & POLLNVAL)
    {
        // POLLNVAL表示文件描述符没有打开，相当于是无效
        std::cout << "Channel handle Event POLLNVAL\n";
    }
    if (revents_ & (POLLNVAL | POLLERR))
    {
        // POLLERR就是发生了某种错误
        if (errCallback_)
            errCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        // POLLPRI表示有高优先级的数据可读
        // POLLRDHUP表示对端描述符已经关闭读
        if (readEventCallback_)
            readEventCallback_(ReceiveTime);
    }
    if (revents_ & POLLOUT)
    {
        if (writeEventCallback_)
            writeEventCallback_();
    }
    eventHandling_ = false;
}

Channel::~Channel()
{
    // 事件处理期间不能进行析构
    assert(!eventHandling_);
}

// // 测试使用
// void Channel::handleRead()
// {
//     char buf[1024];
//     size_t n = read(fd_, buf, sizeof buf);
//     if (n > 0)
//     {
//         std::cout << "read from fd:" << fd_ << buf << std::endl;
//     }
//     else if (n == 0)
//     {
//         std::cout << "close\n";
//         disableAll();
//         loop_->removeChannel(this);
//     }
//     else
//     {
//         std::cout << "some eroro has happened\n";
//     }
// }

void Channel::remove()
{
    loop_->removeChannel(this);
}
