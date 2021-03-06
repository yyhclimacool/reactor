#include "epoll_poller.h"

#include <glog/logging.h>
#include <sys/epoll.h>
#include <sys/poll.h>

#include "channel.h"

static_assert(EPOLLIN == POLLIN, "EPOLLIN equals to POLLIN");
static_assert(EPOLLPRI == POLLPRI, "EPOLLPRI equals to POLLPRI");
static_assert(EPOLLOUT == POLLOUT, "EPOLLOUT equals to POLLOUT");
static_assert(EPOLLRDHUP == POLLRDHUP, "EPOLLRDHUP equals to POLLRDHUP");
static_assert(EPOLLERR == POLLERR, "EPOLLERR equals to POLLERR");
static_assert(EPOLLHUP == POLLHUP, "EPOLLHUP equals to POLLHUP");

using namespace muduo;

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
} // namespace

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , _epollfd(epoll_create1(EPOLL_CLOEXEC))
    , _events(kInitEventListSize) {
  if (_epollfd < 0) {
    LOG(FATAL) << "create epoll fd failed, loop=" << loop;
  }
}

EPollPoller::~EPollPoller() { close(_epollfd); }

Timestamp EPollPoller::poll(int timeout_ms, std::vector<Channel *> *active_channels) {
  DLOG(INFO) << "total fd num=" << _channels.size() << " in epoll[" << this << "]";
  int  num_events = epoll_wait(_epollfd, _events.data(), static_cast<int>(_events.size()), timeout_ms);
  int  saved_errno = errno;
  auto now = Timestamp::now();
  if (num_events > 0) {
    DLOG(INFO) << "num_events=" << num_events << " happend in epoll[" << this << "]";
    fill_active_channels(num_events, active_channels);
    if (static_cast<size_t>(num_events) == _events.size()) {
      _events.resize(_events.size() * 2);
    }
  } else if (num_events == 0) {
    LOG(INFO) << "nothing happend in epoll[" << this << "]";
  } else {
    // error happened
    if (saved_errno != EINTR) {
      errno = saved_errno;
      LOG(WARNING) << "caused an error_no=" << saved_errno << " in epoll[" << this << "]";
    }
  }

  return now;
}

void EPollPoller::fill_active_channels(int num_events, std::vector<Channel *> *active_channels) const {
  assert(static_cast<size_t>(num_events) <= _events.size());
  for (int i = 0; i < num_events; ++i) {
    Channel *ch = static_cast<Channel *>(_events[ i ].data.ptr);
#ifndef NDEBUG
    int  fd = ch->fd();
    auto it = _channels.find(fd);
    assert(it != _channels.end());
    assert(it->second == ch);
#endif
    ch->set_revents(_events[ i ].events);
    active_channels->push_back(ch);
  }
}

void EPollPoller::update_channel(Channel *ch) {
  assert_in_loop_thread();
  const int index = ch->index();
  DLOG(INFO) << "fd=" << ch->fd() << ", events=" << ch->events() << ", index=" << ch->index();
  if (index == kNew || index == kDeleted) {
    int fd = ch->fd();
    // if it's a new channel.
    if (index == kNew) {
      assert(_channels.find(fd) == _channels.end());
      _channels[ fd ] = ch;
    } else { // if this channel is marked deleted, but not real deleted from map.
      assert(_channels.find(fd) != _channels.end());
      assert(_channels[ fd ] == ch);
    }
    update(EPOLL_CTL_ADD, ch);
    // TODO : check
    ch->set_index(kAdded);
  } else {
    int fd = ch->fd();
    assert(_channels.find(fd) != _channels.end());
    assert(_channels[ fd ] == ch);
    assert(index == kAdded);
    if (ch->is_none_event()) {
      // marked as non-event, delete it and mark it's index as kDeleted.
      update(EPOLL_CTL_DEL, ch);
      ch->set_index(kDeleted);
    } else {
      // not non-event, but actually it's event has changed, so update to epoll.
      update(EPOLL_CTL_MOD, ch);
    }
  }
}

// ?????????channel???index?????????channel?????????
// ??????channel?????????????????????????????????????????????channel???epoll poller???????????????????????????epollfd??????????????????channel map?????????
// ??????channel???kAdded?????????????????????????????????
// ????????????channel??????kNew?????????
void EPollPoller::remove_channel(Channel *ch) {
  assert_in_loop_thread();
  int fd = ch->fd();
  assert(_channels.find(fd) != _channels.end());
  assert(_channels[ fd ] == ch);
  assert(ch->is_none_event());
  int index = ch->index();
  assert(index == kAdded || index == kDeleted);
  size_t n = _channels.erase(fd);
  (void)n;
  assert(n == 1);
  if (index == kAdded) {
    update(EPOLL_CTL_DEL, ch);
  }
  ch->set_index(kNew);
}

const char *EPollPoller::operation_to_string(int op) {
  switch (op) {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR OP");
      return "Invalid Operation";
  }
}

void EPollPoller::update(int op, Channel *ch) {
  struct epoll_event event;
  memset(&event, 0x00, sizeof(event));
  event.events = ch->events();
  event.data.ptr = ch;
  int fd = ch->fd();
  DLOG(INFO) << "epoll_ctl op=" << operation_to_string(op) << ", fd=" << fd << ", event=" << ch->events_to_string();
  if (epoll_ctl(_epollfd, op, fd, &event) < 0) {
    if (op == EPOLL_CTL_DEL) {
      LOG(WARNING) << "epoll_ctl_del for fd=" << fd << " failed.";
    } else {
      LOG(WARNING) << "epoll_ctl op=" << operation_to_string(op) << " for fd=" << fd << " failed.";
    }
  }
}

Poller *Poller::new_default_poller(EventLoop *loop) { return new EPollPoller(loop); }
