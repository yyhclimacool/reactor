#pragma once

#include <vector>

#include "poller.h"

struct epoll_event;

namespace muduo {

// epoll作为poller的实现
class EPollPoller : public Poller {
public:
  EPollPoller(EventLoop *);
  ~EPollPoller() override;

  Timestamp poll(int timeout_ms, std::vector<Channel *> *active_channels) override;

  void update_channel(Channel *) override;
  void remove_channel(Channel *) override;

private:
  static const int   kInitEventListSize = 16;
  static const char *operation_to_string(int op);
  void               fill_active_channels(int num_events, std::vector<Channel *> *active_channels) const;
  void               update(int op, Channel *ch);

  int                             _epollfd;
  std::vector<struct epoll_event> _events;
};

} // namespace muduo
