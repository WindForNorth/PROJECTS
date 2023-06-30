#include <sys/time.h>
#include "Timestamp.h"

Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t seconds = tv.tv_sec;
    return Timestamp(tv.tv_sec * kMicroSecondsPerSecond + tv.tv_usec);
    // 这个时间相当于是求从纪元元年开始到现在经过的微秒数
    // gettimeofday通过timeval结构返回纪元元年到现在经过的整秒数和不足一秒的微秒数
}

std::string Timestamp::toFormatTime()
{
    char buf[100];
    struct tm time_;
    int64_t seconds = microSecondsSinceEpoch_ / 1000;
    gmtime_r(&seconds, &time_);
    int add_day = time_.tm_hour + 8 >= 24 ? 1 : 0;
    snprintf(buf, sizeof buf, "%d:%d:%d:%d:%d:%d",
             time_.tm_yday + 1900,
             time_.tm_mon + 1,
             time_.tm_mday + add_day,
             (time_.tm_hour + 8) % 24,
             time_.tm_min,
             time_.tm_sec);
    return buf;
}