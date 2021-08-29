#pragma once

#include "timestamp.h"

#include <map>
#include <vector>

class Channel;
class EventLoop;

class Poller {
public:
    Poller(EventLoop *loop);
    virtual ~Poller();
    virtual Timestamp Poll(int timeout_ms, std::vector<Channel *> *active_channels) = 0;
    virtual void UpdateChannel(Channel *) = 0;
    virtual void RemoveChannel(Channel *) = 0;
    virtual bool HasChannel(Channel *) const ;

    static Poller *NewDefaultPoller(EventLoop *loop);
    void AssertInLoopThread() const;
protected:
    std::map<int, Channel *> _channels;
private:
    EventLoop *_owner_loop;
};
