#pragma once

#include <any>
#include <boost/noncopyable.hpp>
#include <string>

#include "buffer.h"
#include "callbacks.h"

// strcut tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo {
class Channel;
class EventLoop;
class Socket;

class TcpConnection : public boost::noncopyable, public std::enable_shared_from_this<TcpConnection> {
public:
private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  EventLoop        *_loop;
  const std::string _name;
  StateE            _state; // TODO: use atomic variable
  bool              _reading;

  // we don't expose those classes to client.
  std::unique_ptr<Socket>  _socket;
  std::unique_ptr<Channel> _channel;
  const InetAddress        _local_addr;
  const InetAddress        _peer_addr;

  ConnectionCallback    _connection_callback;
  MessageCallback       _message_callback;
  WriteCompleteCallback _write_complete_callback;
  HighWaterMarkCallback _high_water_mark_callback;
  CloseCallback         _close_callback;

  size_t   _high_water_mark;
  Buffer   _input_buffer;
  Buffer   _output_buffer; // FIXME: use list<Buffer> as output buffer
  std::any _context;
};
} // namespace muduo
