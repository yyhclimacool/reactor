#include "poller.h"
#include "channel.h"
#include "event_loop.h"

Poller::Poller(EventLoop *loop) : _owner_loop(loop) {}

Poller::~Poller() = default;

// 只能在loop线程中运行
bool Poller::HasChannel(Channel *ch) const {
    AssertInLoopThread();
    auto it = _channels.find(ch->fd());
    return it != _channels.end() && it->second == ch;
}


inline void Poller::AssertInLoopThread() const {
    return _owner_loop->AssertInLoopThread();
}
