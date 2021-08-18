#pragma once

#include "timestamp.h"

#include <map>

class Channel;

class Poller {
public:
    Poller(EventLoop *loop);
    virtual ~Poller();
    virtual Timestamp poll(int timeout_ms, std::vector<Channel *> active_channels) = 0;
    virtual void UpdateChannel(Channel *) = 0;
    virtual void RemoveChannel(Channel *) = 0;
    virtual bool HasChannel(Channel *) = 0;

    static Poller *NewDefaultPoller(EventLoop *loop);
    void AssertInLoopThread() const {
        return _owner_loop->AssertInLoopThread();
    }
protected:
    std::map<int, Channel *> _channels;
private:
    EventLoop *_owner_loop;
};
