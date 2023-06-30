#include "../Poller.h"
#include "PollPoller.h"
#include "EpollPoller.h"

Poller * Poller::defaultPoller(bool ifUsePoller , eventsLoop * loop)
{
    if(ifUsePoller)
    {
        return new PollPoller(loop);
    } else {
        return new EpollPoller(loop);
    }   
}