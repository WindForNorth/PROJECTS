#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H
#include "../base/noncopyable.h"
#include "CallBack.h"
#include  <set>
#include <memory>
#include <vector>
#include "Timestamp.h"
#include "Channel.h"
class Timer;
class eventsLoop;
class TimerID;
//组织定时器的类
class TimerQueue : noncopyable
{
public:
    TimerQueue(eventsLoop * loop);
    ~TimerQueue() = default;
    TimerID addTimer(const TimerCallBackType & cb , Timestamp when , double interval);
    void cancel(TimerID tmid_);
    void reSetTimerFd(Timestamp expiration , int timerfd);
    timespec restTimeFromNow(Timestamp expiration);
private:
    void readTimerFd();
    void addTimerInLoop(std::shared_ptr<Timer>);
    void cancelTimerInLoop(TimerID tmid_);
    
    typedef std::pair<Timestamp , std::shared_ptr<Timer>> Entry;
    typedef std::set<Entry> TimerListType;

    typedef std::shared_ptr<Timer> activeTimerType;
    typedef std::set<activeTimerType> activeTimerTypeSet;

    void reset(Timestamp now , std::vector<Entry> & );
    void handleReand();                                                 //超时时调用
    std::vector<Entry> getExpired(Timestamp now);                       //移除超时定时器
    // void reset(const std::vector<Entry> & expired , Timestamp now);     //
    bool insert(std::shared_ptr<Timer> & timer);                        //插入定时器
    const int timerfd_;                                                 //timerfd
    eventsLoop * loop_;                                                 //owner loop
    Channel TimerfdChannel_;                                            //Channel for timerfd
    TimerListType timers_;                                              //时间队列，按照超时时间排序，方便取出超时定时器
    activeTimerTypeSet activeTimersSet_;                                //当前活跃队列，为了方便cancel
};


#endif