#include "LogFile.h"
#include <stdio.h>
#include <string.h>
LogFile::LogFile(const std::string & basename , int maxSingleFileSize , int flushInterval) :
    filebasename_(basename),
    maxSingleFileSize_(maxSingleFileSize),
    flushInterval_(flushInterval),
    mutex_(),
    lastFlush_(0),
    timeOfStartThisFile_(0),
    writenBytes_(0)
{
    switchFile();
}

void LogFile::flush()
{
    mutexLockGuard lock(mutex_);
    fflush(filestream_);
}
void LogFile::unlock_flush()
{
    fflush(filestream_);
}

LogFile::~LogFile()
{

}

void LogFile::switchFile()
{
    time_t now_time = 0;
    writenBytes_ = 0;
    std::string fullfilename = getLogName(filebasename_ , now_time);
    timeOfStartThisFile_ = now_time / secondsOfDay_ * secondsOfDay_;
    lastFlush_ = timeOfStartThisFile_;
    filestream_ = fopen(fullfilename.c_str() , "ae");
    assert(filestream_);
    bzero(streamBuffer_ , sizeof streamBuffer_);
    setbuffer(filestream_ , streamBuffer_ , sizeof streamBuffer_);
}

void LogFile::write(const char * log , size_t size)
{
    mutexLockGuard lock(mutex_);
    if(size != 0) writeFile(log , size);
    if(getWriteBytes() >= maxSingleFileSize_)
    {
        fclose(filestream_);
        filestream_ = nullptr;
        switchFile();
    }
    else
    {
        time_t now_time = time(nullptr);
        time_t startOfThsiFile = now_time / secondsOfDay_ * secondsOfDay_;
        if(startOfThsiFile != timeOfStartThisFile_)
        {
            switchFile();
        }
        else if (now_time - lastFlush_ >= flushInterval_)
        {
            unlock_flush();
            lastFlush_ = now_time;
        }
    }
}

std::string LogFile::getLogName(const std::string & filebasename, time_t & now_time)
{
    //日志全名：basename.日期.主机名.log
    std::string fullFilename = filebasename;
    fullFilename.reserve(filebasename.length() + 64);

    char timebuff[32];
    time_t _8hour = 8 * 60 * 60;
    struct tm tm;
    time(&now_time);
    now_time += _8hour;
    gmtime_r(&now_time , &tm);
    strftime(timebuff , sizeof timebuff , ".%Y%m%d-%H%M%S." , &tm);
    fullFilename += timebuff;

    char hostname[64];
    gethostname(hostname , sizeof hostname);
    fullFilename += hostname;

    fullFilename += ".log";
    return fullFilename;
}

void LogFile::writeFile(const char * log , size_t size)
{
    size_t writen = 0;
    while(writen != size)
    {

        size_t bytes = fwrite(log + writen , 1 , size - writen , filestream_);
        if(bytes > 0)
        {
            writen += bytes;
        }
        else
        {
            printf("write err\n");
        }
    }
    writenBytes_ += size;
}
