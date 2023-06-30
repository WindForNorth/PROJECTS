#ifndef EPOLLPOLLER_H
#define EPOLLPOLLER_H
#include "../Poller.h"
struct epoll_event;
class eventsLoop;
class Channel;



class EpollPoller : public Poller 
{
public:
    typedef std::vector<struct epoll_event> EpollEventsListType;                    //epoll_wait需要第二个参数需要地址连续的空间
private:
    static const int initEventsSize;
    static const int kAdded;        //表示已经添加了的状态
    static const int kDeled;        //表示已经删除的状态，应该是为了复用Channel
    static const int kNew;          //表示未添加状态
    int epollfd_;
    EpollEventsListType epollEventsList_;       //存放所有epollfd和其对应的Channel，方便在不需要时进行删除，使用std::map<int , Channel*>
    
    void fillActiveChannels(int numOfEvents , ChannelList * activeChannels) const;
    void updateEvent(int operation , Channel *);
    
public:
    EpollPoller(eventsLoop * loop);
    ~EpollPoller();
    virtual Timestamp poll(int timeOutMs , ChannelList * activeChannels);  //poll start
    virtual void removeChannel(Channel *);                  //从map中移除一个Channel
    virtual void updateChannels(Channel * channel);
};



#endif