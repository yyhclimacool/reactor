#include "poller.h"

#include "channel.h"
#include "event_loop.h"

using namespace muduo;

Poller::Poller(EventLoop *loop)
    : _owner_loop(loop) {}

Poller::~Poller() = default;

// 只能在loop线程中运行
bool Poller::has_channel(Channel *ch) const {
  assert_in_loop_thread();
  auto it = _channels.find(ch->fd());
  return it != _channels.end() && it->second == ch;
}

void Poller::assert_in_loop_thread() const { return _owner_loop->assert_in_loop_thread(); }