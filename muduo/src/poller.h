#pragma once

#include <map>
#include <vector>

#include "timestamp.h"

namespace muduo {

class Channel;
class EventLoop;

class Poller {
public:
  Poller(EventLoop *loop);
  virtual ~Poller();
  virtual Timestamp poll(int timeout_ms, std::vector<Channel *> *active_channels) = 0;

  // think about their thread-safety ?
  virtual bool has_channel(Channel *) const;
  virtual void update_channel(Channel *) = 0;
  virtual void remove_channel(Channel *) = 0;

  static Poller *new_default_poller(EventLoop *loop);
  void           assert_in_loop_thread() const;

protected:
  // fd to Channel pointer
  // TODO: think how to make sure its thread-safety ?
  // Answer: you can modify _channels only in loop thread, but by what means ?
  std::map<int, Channel *> _channels;

private:
  EventLoop *_owner_loop;
};

} // namespace muduo
