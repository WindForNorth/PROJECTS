#ifndef CHANNEL_H
#define CHANNEL_H
#include "../base/noncopyable.h"
#include <functional>
class eventsLoop;
class Timestamp;
//用于管理描述符事件
class Channel : noncopyable
{
public:
    typedef std::function<void ()> EventCallback;
    typedef std::function<void (Timestamp)> ReadEventCallBackType;
    Channel (eventsLoop * loop , int fd);
    ~Channel ();
    void handleEvent(Timestamp ReceiveTime);
    void setReadCallback(const ReadEventCallBackType & cb)  { readEventCallback_ = cb;  }
    void setWriteCallback(const EventCallback & cb) { writeEventCallback_ = cb; }
    void setErrCallback(const EventCallback & cb)   { errCallback_ = cb;        }
    void setCloseCallBack(const EventCallback & cb) { closeCallBack_ = cb;      }
    int fd() const { return fd_; }
    int events() { return events_; }
    int revents() { return revents_; }
    void set_events(int events) { events_ |= events; }
    void set_revents(int events) {revents_ |= events; update(); }
    bool ifnoneEvents()   { return kNoneEvent == events_;       }
    void enableReading()  {events_ |= kReadEvent; update();     }
    void disableAll()     { events_ = kNoneEvent; update();     }           //让所用事件失效
    void disableReading() { events_ &= ~kWriteEvent; update();  }
    void enableWriting()  { events_ |= kWriteEvent; update();   }           //可写
    //单单禁止读，主要是因为水平触发只要缓冲区可写就会一直触发，导致poll永远立马返回，于是在不需要写的时候禁止即可
    void disableWriting() { events_ &= ~kWriteEvent ; update(); }           
    bool isWriting() { return events_ & kWriteEvent; }
    void remove();

    //test
    // void handleRead();
    eventsLoop * ownerLoop() { return loop_; }
    int getindex() { return index_; }
    void set_index(int idx) { index_ = idx; }

private:
    void update();
    //事件类型
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    bool eventHandling_;                    //事件处理进行的标志
    int events_;                            //Channel关心的IO事件
    int revents_;                           //Channel当前关心的IO事件
    const int fd_;                          //负责的文件描述符
    eventsLoop * loop_;                     //owner loop
    int index_;                             //用于记录自己负责的fd在Poller::pollfds_数组中的下标方便快速定位

    //三种回调
    ReadEventCallBackType readEventCallback_;
    EventCallback writeEventCallback_;
    EventCallback errCallback_;             //错误回调
    EventCallback closeCallBack_;           //关闭连接时回调
};

#endif