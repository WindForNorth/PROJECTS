#include "PollPoller.h"
#include "../Channel.h"
#include "../Timestamp.h"
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <poll.h>



//up_cast的安全转换
template<typename To , typename From>
inline To implicit_cast(const From & f)
{
    return f;
}

PollPoller::PollPoller(eventsLoop * loop)  : Poller(loop) {}
PollPoller::~PollPoller() = default;                            //析构函数要释放掉struct pollfd等内容，头文件中没有包含相关定义的文件就会报错
//更新维护pollfd，注册新的fd
void PollPoller::updateChannels(Channel * channel){
    Poller::assertInLoopThread();
    if(channel->getindex() < 0){
        //此时说明该fd还没有注册
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        //添加新注册fd到pollfds_数组，并且通过channel来记住相关内容
        pfd.events = static_cast<short>(channel->events());
        pfd.fd = channel->fd();
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = pollfds_.size() - 1;
        channel->set_index(idx);
        //添加到map
        channels_[pfd.fd] = channel;
        std::cout << "fd regester success\n";
    } else {
        //此时说明该channel已经注册过了,于是做更新操作，此时channel的index就发挥作用了
        //为了快速修改fd的监测事件
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->getindex();
        assert(idx >= 0 && idx < pollfds_.size());
        struct pollfd & pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if(channel->ifnoneEvents()) {
            pfd.fd = -channel->fd() - 1;  //表示忽略该fd，poll回忽略fd < 0的结构，减一则是为了保证当描述符为0时也能被忽略
        }
    }
}



void PollPoller::removeChannel(Channel * channel)
{
    Poller::assertInLoopThread();
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->ifnoneEvents());
    int idx = channel->getindex();
    assert(idx >= 0 && idx < pollfds_.size());
    const struct pollfd & pfd = pollfds_[idx]; 
    assert(-channel->fd() - 1== pfd.fd && channel->events() == pfd.events);
    size_t n = channels_.erase(channel->fd());                          //从Channl列表中删除
    assert(n == 1);
    if(implicit_cast<size_t>(idx) == pollfds_.size() - 1)
    {
        pollfds_.pop_back();
    } else {
        int lastElemidx = pollfds_.back().fd;
        if(lastElemidx < 0){
            lastElemidx = -lastElemidx - 1;
        }
        std::iter_swap(pollfds_.begin() + idx , pollfds_.end() - 1);    //巧妙利用了交换迭代器的算法，将删除操作复杂度降低到O(1)
        pollfds_.pop_back();                                            //从监听列表中删除
        channels_[lastElemidx]->set_index(idx);
    }
}


void PollPoller::fillActiveChannels(int numEvents , ChannelList * activeChannels) const { 
    for(auto it = pollfds_.begin(); it != pollfds_.end() && numEvents > 0; ++it){
        if(it->revents > 0){
            --numEvents;  //控制数量来减少不必要的遍历，因为被监听的fd可能很多，但是有的没有事件发生
            //Channel控制某fd的事件分发，当某fd发生事件时，通过使用map结构快速查找对应的Channel
            ChannelMap::const_iterator iterator = channels_.find(it->fd);
            assert(iterator != channels_.end());
            Channel * channel = iterator->second;
            assert(channel->fd() == it->fd);
            channel->set_revents(it->revents);  //将其当前活跃事件写到变量中
            activeChannels->push_back(channel); //加入当前活跃队列，于是可以通过这个队列处理发生事件的fd上的事件
        }
    }
}


Timestamp PollPoller::poll(int timeOutMs , ChannelList * activeChannles){
    int numEvents = ::poll(&*pollfds_.begin() , pollfds_.size() , timeOutMs);
    Timestamp receiveTime(Timestamp::now());
    if(numEvents > 0){
        fillActiveChannels(numEvents , activeChannles);
    } else if (numEvents == 0) {
        std::cout << "nothing happened\n";
    } else {
        std::cout << "an error has happened at Poller.h line 5\n";
    }
    return receiveTime;
}