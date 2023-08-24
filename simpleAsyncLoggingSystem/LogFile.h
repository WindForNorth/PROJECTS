#ifndef LOGFILE_H
#define LOGFILE_H
#include <string>
#include "mutex.h"

class LogFile
{
private:
    FILE * filestream_;
    char streamBuffer_[1024 * 2];
    const std::string filebasename_;
    mutexLock mutex_;
    const int maxSingleFileSize_;
    int writenBytes_;

    const int flushInterval_;
    time_t lastFlush_;
    time_t timeOfStartThisFile_;
    const static int secondsOfDay_ = 60 * 60 * 24;
    
    static std::string getLogName(const std::string & filebasename , time_t & now_time);
    void writeFile(const char * log , size_t size);
    int getWriteBytes() { return writenBytes_; }
public:
    LogFile(const std::string & filename , int maxSingleFileSize , int flushInterval = 3);
    ~LogFile();
    void write(const char * log , size_t size);
    void flush();
    void unlock_flush();
    void switchFile();

};

#endif