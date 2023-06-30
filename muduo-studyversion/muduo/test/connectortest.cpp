#include "../net/Connector.h"
#include "../net/eventsLoop.h"
#include "../net/IPAddr.h"
#include <iostream>
std::shared_ptr<Connector>connector[1024];
void onConnection(int sockfd)
{
    std::cout << "sockfd is " << sockfd << "\n";
}

int main(int argc , char * argv[])
{
    IPAddr addr("127.0.0.1" , 8888);
    eventsLoop loop;
    for(int i = 0; i < 1024; ++i)
    {
        connector[i].reset(new Connector(&loop , addr));
        connector[i]->setConnectionCallBack(onConnection);
    }
    
    for(int i = 0; i < 1024; ++i)
    {
        connector[i]->start();
    }
    loop.loop();
    return 0;
}

