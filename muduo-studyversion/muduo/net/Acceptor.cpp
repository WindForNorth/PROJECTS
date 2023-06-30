#include "Acceptor.h"
#include "eventsLoop.h"
#include "IPAddr.h"
#include "socketOperations.h"
#include <iostream>
#include <fcntl.h>

//Acceptor的构造和listen函数执行服务端TCP通信的基础流程：socket、bind、listen
//注意初始化列表的初始化顺序不是按照列表顺序来的，是按照声明顺序来的
Acceptor::Acceptor(eventsLoop * loop , const IPAddr & listenAddr):
    loop_(loop),
    listening_(false),
    acceptorSocket_(createNonBlockingSocket()),
    acceptorSocketChannel_(loop , acceptorSocket_.fd()),
    freefd_(open("/dev/null" , O_RDONLY | O_CLOEXEC))
{
    std::cout << "listenfd_: " << acceptorSocket_.fd() << std::endl;
    acceptorSocketChannel_.setReadCallback(std::bind(&Acceptor::handleRead , this));
    acceptorSocket_.setReuseAddr(true);
    acceptorSocket_.setReusePort(true);
    acceptorSocket_.bindAddr(listenAddr);
}

void Acceptor::listen(){
    loop_->assertInLoopThread();                //Reactor模型IO线程的监听操作
    acceptorSocket_.listen();                   //
    acceptorSocketChannel_.enableReading();     //套接字描述符生效
}

//处理acceptorChannel的时候回调
void Acceptor::handleRead(){
    loop_->assertInLoopThread();
    IPAddr addr(0);             //最终存储用户套接字地址结构
    int connfd = acceptorSocket_.accept(addr);
    if(connfd >= 0){
        if(NewConnectionCallBack_) NewConnectionCallBack_(connfd , addr);
        else {
            //如果回调不存在就关闭套接字，说明使用该Acceptor的TcpServer至少需要设置一个默认的可读事件处理方法
            closefd(connfd);
        }
    }
    else
    {
        std::cout << "sys_error in Acceptor::handleRead\n";
        if(errno == EMFILE)
        {
            //描述符耗尽
            close(freefd_);
            //不保证一定能正确运行，多线程下可能其他线程需要打开一个描述符大致这个描述符在关闭之后立马被使用
            freefd_ = acceptorSocket_.accept(addr);
            close(freefd_);
            freefd_ = open("/dev/null" , O_RDONLY | O_CLOEXEC);
        }
    }
}

Acceptor::~Acceptor(){

}
