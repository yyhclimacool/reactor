#pragma once

#include <functional>
#include <memory>

#include "timestamp.h"

namespace muduo {

class EventLoop;

// A selectable I/O channel
//
// this class dosen't own the file descriptor
// a file descriptor could be a socket, eventfd, timerfd or a signalfd
// 不拥有fd就意味着不负责管理fd相关的OS资源，Channel析构时不负责close该fd
class Channel {
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  // Channel不拥有fd，在析构的时候不close(fd)
  Channel(EventLoop *loop, int fd);
  ~Channel();

  // 在event_loop.loop中被调用
  void handle_event(Timestamp receive_time);

  void set_read_callback(ReadEventCallback rcb) { _read_callback = std::move(rcb); }
  void set_write_callback(EventCallback wcb) { _write_callback = std::move(wcb); }
  void set_close_callback(EventCallback ccb) { _close_callback = std::move(ccb); }
  void set_error_callback(EventCallback ecb) { _error_callback = std::move(ecb); }

  // TODO: check this
  void tie(const std::shared_ptr<void> &);

  int  fd() const { return _fd; }
  int  events() const { return _events; }
  void set_revents(int revt) { _revents = revt; }

  bool is_none_event() const { return _events == kNoneEvent; }
  bool is_writing() const { return _events & kWriteEvent; }
  bool is_reading() const { return _events & kReadEvent; }

  void enable_reading() {
    _events |= kReadEvent;
    update();
  }
  void disable_reading() {
    _events &= ~kReadEvent;
    update();
  }
  void enable_writing() {
    _events |= kWriteEvent;
    update();
  }
  void disable_writing() {
    _events &= ~kWriteEvent;
    update();
  }
  void disable_all() {
    _events = kNoneEvent;
    update();
  }

  int  index() const { return _index; }
  void set_index(int idx) { _index = idx; }

  std::string revents_to_string() const;
  std::string events_to_string() const;

  // TODO: check this
  void disable_log_hup() { _log_hup = false; }

  EventLoop *owner_loop() { return _loop; }
  // 将此Channel从epollpooler中删除
  void remove();

private:
  static std::string events_to_string(int fd, int ev);

  void update();
  void handle_event_with_guard(Timestamp receive_time);

private:
  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop *_loop;
  const int  _fd;
  int        _events;
  int        _revents; // received event types
  int        _index;   // used by poller.
  bool       _log_hup;

  std::weak_ptr<void> _tie;
  bool                _tied;
  bool                _event_handling;
  bool                _added_to_loop;

  ReadEventCallback _read_callback;
  EventCallback     _write_callback;
  EventCallback     _close_callback;
  EventCallback     _error_callback;
};

} // namespace muduo
