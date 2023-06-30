#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <inttypes.h>
#include <string>
class Timestamp
{
public:
    Timestamp() : microSecondsSinceEpoch_(0)
    {
    }
    explicit Timestamp(int64_t microSecondsSinceEpoch) : microSecondsSinceEpoch_(microSecondsSinceEpoch)
    {
    }
    ~Timestamp() = default;
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    static const int kMicroSecondsPerSecond = 1000 * 1000; // 每秒微秒数
    static Timestamp now();
    std::string toFormatTime();

private:
    int64_t microSecondsSinceEpoch_; // 微秒间隔（应该叫绝对超时时间），Unix纪元元年开始的微秒数
};

inline bool operator<(Timestamp l, Timestamp r) { return l.microSecondsSinceEpoch() < r.microSecondsSinceEpoch(); }
inline bool operator==(Timestamp l, Timestamp r) { return l.microSecondsSinceEpoch() == r.microSecondsSinceEpoch(); }
inline bool operator<=(Timestamp l, Timestamp r) { return l.microSecondsSinceEpoch() <= r.microSecondsSinceEpoch(); }
// 增加超时时间
inline Timestamp addTime(Timestamp stamp, double seocnds)
{
    int64_t delta = static_cast<int64_t>(seocnds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(delta + stamp.microSecondsSinceEpoch());
}

#endif