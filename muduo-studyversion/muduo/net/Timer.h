#ifndef TIMER_H
#define TIEMr_H

#include "../base/noncopyable.h"
#include "CallBack.h"
#include "Timestamp.h"
#include <atomic>
#include <iostream>
// 单个定时器

class Timer : noncopyable
{
public:
    Timer(TimerCallBackType cb, double interval, Timestamp expiration) : interval_(interval),
                                                                         timercallback_(std::move(cb)),
                                                                         // sequence_(++sequenceNum),  //const类型只能在初始化列表里初始化
                                                                         expiration_(expiration),
                                                                         reapeat_(interval > 0.0)
    {
    }
    Timer() : interval_(0.0) {}
    ~Timer() {}
    // const int64_t sequence(){ return sequence_ ;}
    Timestamp expiration() { return expiration_; }
    void run()
    {
        if (timercallback_)
            timercallback_();
        else
            std::cout << "Timer's Callback is null\n";
    }
    bool isNeedReapeat() { return reapeat_; }
    void restart(Timestamp now)
    {
        expiration_ = addTime(now, interval_);
    }
    bool ifnull()
    {
        return timercallback_ == nullptr;
    }

private:
    const double interval_;           // 时间间隔
    TimerCallBackType timercallback_; // 超时回调
    // const int64_t sequence_;            //在定时器队列中的顺位唯一标识
    Timestamp expiration_; // 超时时间
    bool reapeat_;
    // static std::atomic<int64_t> sequenceNum;    //全局定时数量记录，采用原子递增
};

#endif