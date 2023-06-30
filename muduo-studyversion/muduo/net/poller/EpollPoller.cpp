#include <sys/epoll.h>
#include <iostream>
#include <assert.h>
#include <unistd.h> //Unix标准库，for close()
#include <string.h>
#include "EpollPoller.h"
#include "../Channel.h"
#include "../Timestamp.h"

const int EpollPoller::initEventsSize = 16;
const int EpollPoller::kAdded = 1;
const int EpollPoller::kDeled = 2;
const int EpollPoller::kNew = -1;
// up_cast的安全转换
template <typename To, typename From>
inline To implicit_cast(const From &f)
{
    return f;
}

EpollPoller::EpollPoller(eventsLoop *loop) : Poller(loop),
                                             epollfd_(epoll_create1(EPOLL_CLOEXEC)),
                                             epollEventsList_(initEventsSize)
{
    if (epollfd_ < 0)
    {
        std::cout << "failed to create epollfd\n";
        abort();
    }
}

EpollPoller::~EpollPoller()
{
    close(epollfd_);
}

void EpollPoller::removeChannel(Channel *channel_)
{
    Poller::assertInLoopThread();
    int fd = channel_->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel_);
    assert(channel_->ifnoneEvents());
    int idx = channel_->getindex();
    assert(idx == kDeled || idx == kAdded);
    size_t n = channels_.erase(fd);
    assert(n == 1);
    if (idx == kAdded)
    {
        updateEvent(EPOLL_CTL_DEL, channel_);
    }
    // remove只是删除在map中的指针记录，
    // Channel的所有权在对应的对象内部
    // 在对象析构前，Channel永远可以复用
    channel_->set_index(kDeled);
}

void EpollPoller::updateChannels(Channel *channel_)
{
    Poller::assertInLoopThread();
    int idx = channel_->getindex();
    if (idx == kNew || idx == kDeled)
    {
        // 注册或者复用
        int fd = channel_->fd();
        if (idx == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel_;
        }
        else
        {
            // 暂时没看到复用在哪，这个kDeled感觉没用
            // 只是提供了一种复用的可能性
            // 比如最开始只关心写事件，当不关心时将其关闭之后又需要
            // 添加可读事件的监听，这样
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel_);
        }
        channel_->set_index(kAdded);
        updateEvent(EPOLL_CTL_ADD, channel_);
    }
    else
    {
        // 修改
        int fd = channel_->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel_);
        assert(idx = kAdded);
        if (channel_->ifnoneEvents())
        {
            // 表示此时不关心任何事件，即取消epoll
            // 使用poll时的操作时将其fd设置为相反数减一让系统忽略
            // 那是因为poll需要持续维护一个vector事件列表，删除的开销比较大
            // 但是epoll更为方便可以直接将维护工作交给内核，内核采用树形结构维护，
            // 删除效率相对较高，于是可以直接进行DEL操作
            updateEvent(EPOLL_CTL_DEL, channel_);
            channel_->set_index(kDeled); // 并没有真正删除，相当于标记删除
        }
        else
        {
            // 修改事件
            updateEvent(EPOLL_CTL_MOD, channel_);
        }
    }
}

Timestamp EpollPoller::poll(int timeOutMs, ChannelList *activeChannels)
{
    assertInLoopThread();
    int nums = epoll_wait(
        epollfd_,
        &*epollEventsList_.begin(),
        static_cast<int>(epollEventsList_.size()),
        timeOutMs);
    Timestamp now(Timestamp::now());
    if (nums > 0)
    {
        std::cout << "epoll " << nums << " evens happened\n";
        fillActiveChannels(nums, activeChannels);
        size_t size = epollEventsList_.size();
        if (implicit_cast<size_t>(nums) == size)
        {
            epollEventsList_.resize(size * 2);
        }
    }
    else if (nums == 0)
    {
        std::cout << "nothing happened\n";
    }
    else
    {
        if (errno != EINTR)
        {
            std::cout << "error happened int EpollPoller::poll\n";
        }
    }

    return now;
}

void EpollPoller::fillActiveChannels(int numOfEvents, ChannelList *activeChannels) const
{
    assert(implicit_cast<size_t>(numOfEvents) <= epollEventsList_.size());
    for (int i = 0; i < numOfEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(epollEventsList_[i].data.ptr);
        int fd = channel->fd();
        auto it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
        channel->set_revents(epollEventsList_[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::updateEvent(int operation, Channel *channel_)
{
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.data.ptr = static_cast<void *>(channel_);
    event.events = channel_->events();
    int fd = channel_->fd();

    if (epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            std::cout << "failed to operate fd: " << fd << " EPOLL_CTL_DEL in EpollPoller::updateEvent\n";
        }
        else
        {
            std::cout << "failed to operate fd: " << fd << " in EpollPoller::updateEvent\n";
            abort(); // 修改和添加失败可能导致更为严重的后果
        }
    }
}