#ifndef CONNECTOR_H
#define CONNECTOR_H
#include "Channel.h"
#include <memory>
#include "IPAddr.h"
class eventsLoop;

class Connector :   noncopyable,
                    public std::enable_shared_from_this<Connector>
{
private:
    typedef std::function<void(int)> newConnectionCallBackType;
    enum state {kDisconnected , kConnecting , kConnected};
    eventsLoop * loop_;
    bool connect_;
    state state_;
    std::unique_ptr<Channel> channel_;
    IPAddr Addr_;
    double retryMs_;
    int retryTimes_;
    static const int maxRetryTimes_;
    newConnectionCallBackType newConnectionCallBack_;

    void resetChannel();
    int removeAndResetChannel();
    void retry(int sockfd);
    void setState(state State_) { state_ = State_; }
    void startInLoop();
    void handleWrite();
    void handleError();
    void connect();
    void connecting(int sockfd);
    void stopInLoop();

public:
    Connector(eventsLoop * loop , const IPAddr & addr);
    ~Connector() = default;
    const static double maxRetryDelayMs_;  //30 * 1000ms
    const static double retryInitMs_ ;    //500ms
    void start();
    void restart();
    void stop();
    void setConnectionCallBack(const newConnectionCallBackType & cb) { newConnectionCallBack_ = cb; }
    void setIPAdrr(const IPAddr & ipadrr) { Addr_ = ipadrr; retryTimes_ = 0; }
};



#endif