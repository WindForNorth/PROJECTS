#ifndef LOG_H
#define LOG_H
#include "../muduo/base/AsyncLogging.h"
// extern AsyncLogging * log;
#define LOG(MSG) if(!_close_log_) { log->append(MSG); }


#endif