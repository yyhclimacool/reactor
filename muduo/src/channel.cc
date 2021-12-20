#include "channel.h"

#include <glog/logging.h>
#include <sys/epoll.h>
#include <sys/poll.h>

#include <sstream>

#include "event_loop.h"

using namespace muduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;
Channel::Channel(EventLoop *loop, int fd)
    : _loop(loop)
    , _fd(fd)
    , _events(0)
    , _revents(0)
    , _index(-1)
    , _log_hup(false)
    , _tied(false)
    , _event_handling(false)
    , _added_to_loop(false) {}

// 可以在其他的线程析构
Channel::~Channel() {
  assert(!_event_handling);
  assert(!_added_to_loop);
  if (_loop->is_in_loop_thread()) {
    assert(!_loop->has_channel(this));
  }
}

// 最重要的函数，在event_loop->loop中调用
void Channel::handle_event(Timestamp receive_time) {
  std::shared_ptr<void> guard;
  if (_tied) {
    guard = _tie.lock();
    if (guard) handle_event_with_guard(receive_time);
  } else {
    handle_event_with_guard(receive_time);
  }
}

void Channel::tie(const std::shared_ptr<void> &obj) {
  _tie = obj;
  _tied = true;
}

std::string Channel::revents_to_string() const { return events_to_string(_fd, _revents); }

std::string Channel::events_to_string() const { return events_to_string(_fd, _events); }

std::string Channel::events_to_string(int fd, int ev) {
  std::ostringstream oss;
  oss << "fd=" << fd << ":";
  if (ev & POLLIN) oss << "IN ";
  if (ev & POLLPRI) oss << "PRI ";
  if (ev & POLLOUT) oss << "OUT ";
  if (ev & POLLHUP) oss << "HUP ";
  if (ev & POLLRDHUP) oss << "RDHUP ";
  if (ev & POLLERR) oss << "ERR ";
  if (ev & POLLNVAL) oss << "NVAL ";

  return oss.str();
}

// 在析构前，需要remove myself
void Channel::remove() {
  assert(is_none_event());
  _added_to_loop = false;
  _loop->remove_channel(this);
}

void Channel::update() {
  _added_to_loop = true;
  _loop->update_channel(this);
}

void Channel::handle_event_with_guard(Timestamp receive_time) {
  _event_handling = true;
  LOG(INFO) << revents_to_string();
  if ((_revents & POLLHUP) && !(_revents & POLLIN)) {
    if (_log_hup) {
      LOG(WARNING) << "fd=" << _fd << " Channel::handle_event() POLLHUP";
    }
    if (_close_callback) _close_callback();
  }

  if (_revents & POLLNVAL) {
    LOG(WARNING) << "fd=" << _fd << " Channel::handle_event() POLLNVAL";
  }

  if (_revents & (POLLERR | POLLNVAL)) {
    if (_error_callback) _error_callback();
  }

  if (_revents & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (_read_callback) _read_callback(receive_time);
  }

  if (_revents & POLLOUT) {
    if (_write_callback) _write_callback();
  }

  _event_handling = false;
}
