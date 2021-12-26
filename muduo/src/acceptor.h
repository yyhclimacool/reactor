#pragma once

#include <boost/noncopyable.hpp>

#include "channel.h"
#include "socket.h"

namespace muduo {

class EventLoop;

class Acceptor : public boost::noncopyable {
public:
  using NewConnectionCallback = std::function<void(int fd, const InetAddress &)>;

  Acceptor(EventLoop *event_loop, const InetAddress &listen_addr, bool reuse_port = false);
  ~Acceptor();

  void set_new_connection_callback(const NewConnectionCallback &cb) { _new_connection_callback = std::move(cb); }
  void listen();
  bool listening() const { return _listening; }

private:
  void                  handle_read();
  EventLoop            *_loop;
  Socket                _accept_socket;
  Channel               _accept_channel;
  NewConnectionCallback _new_connection_callback;
  bool                  _listening;
  int                   _idlefd;
};
} // namespace muduo