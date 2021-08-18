#include "poller.h"
#include "epoll_poller.h"

Poller *Poller::NewDefaultPoller(EventLoop *loop) {
    return new EPollPoller(loop);
}
