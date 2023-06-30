#ifndef TIMEID_H
#define TIMEID_H
#include <memory>
class Timer;
//定时器的标识类，用于记住Timer在TimeQueue组织中的位置，方便快速删除
class TimerID
{
public:
    TimerID(const std::shared_ptr<Timer> timer):
        timer_(std::move(timer))
    {

    }
    TimerID(const TimerID & tmid) = default;
    TimerID() {}
    ~TimerID() = default;
    std::weak_ptr<Timer>  timerPtr() { return timer_; }
private:
    std::weak_ptr<Timer> timer_;
};

#endif