#pragma once

#include "poller.h"

#include <vector>

struct epoll_event;

// epoll作为poller的实现
class EPollPoller : public Poller {
public:
    EPollPoller(EventLoop *);
    ~EPollPoller() override;

    Timestamp Poll(int timeout_ms, std::vector<Channel *> *active_channels) override;

    void UpdateChannel(Channel *) override;
    void RemoveChannel(Channel *) override;
private:

    static const int kInitEventListSize = 16;
    static const char *OperationToString(int op);
    void FillActiveChannels(int num_events, std::vector<Channel *> *active_channels) const;
    void Update(int op, Channel *ch);

    int _epollfd;
    std::vector<struct epoll_event> _events;
};
