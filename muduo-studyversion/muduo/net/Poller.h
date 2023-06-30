#ifndef POLLER_H
#define POLLER_H

#include "../base/noncopyable.h"
#include <vector>
#include <map>
using std::vector;
class eventsLoop;
class Channel;
class Timestamp;

class Poller : noncopyable
{
public:
    static Poller * defaultPoller(bool ifUsePoller , eventsLoop *);
    typedef vector<Channel *> ChannelList;
    Poller(eventsLoop * loop) : ownerLoop_(loop) 
    {}
    virtual ~Poller() {}


    //修改关心的IO事件
    virtual void updateChannels(Channel * channel) = 0;
    void assertInLoopThread() ;//{ ownerLoop_->assertInLoopThread(); }
    virtual void removeChannel(Channel *) = 0;                  //从map中移除一个Channel

    virtual Timestamp poll(int timeOutMs , ChannelList * activeChannels) = 0;  //poll start

private:
    eventsLoop * ownerLoop_;
protected:
    typedef std::map<int , Channel *> ChannelMap;   //记录当前已经注册监听的fd和其对应的Channel
    ChannelMap channels_;
};

#endif