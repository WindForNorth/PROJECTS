#ifndef POLLPOLLER_H
#define POLLPOLLER_H
#include "../Poller.h"
class eventsLoop;
struct pollfd;

class PollPoller : public Poller 
{
private:
    typedef vector<struct pollfd> PollFdList;       //用来poll时记录事件的，但是不同于epoll的是，poll要事先准备好pollfd结构，也就是说，注册的fd也要添加到这里
    void fillActiveChannels(int numOfEvents , ChannelList * activeChannels) const;
    PollFdList pollfds_;
public:
    PollPoller(eventsLoop * loop);
    ~PollPoller();

    virtual Timestamp poll(int timeOutMs , ChannelList * activeChannels);  //poll start
    virtual void updateChannels(Channel * channel);
    virtual void removeChannel(Channel *);                  //从map中移除一个Channel
};



#endif