#pragma once

#include <atomic>
#include <boost/noncopyable.hpp>
#include <memory>
#include <string>

namespace muduo {

class EventLoop;
class InetAddress;
class Acceptor;

// 先实现TcpServer用来接受连接
class TcpServer : public boost::noncopyable {
public:
  TcpServer(EventLoop *acceptor_loop, const InetAddress &listen_addr, const std::string &name);
  ~TcpServer();

  // thread safe
  void start();

  const std::string  ip_port() const { return _ip_port; }
  const std::string &name() const { return _name; }
  EventLoop         *get_loop() const { return _acceptor_loop; }

private:
  void new_connection(int sockfd, const InetAddress &peer_addr);

private:
  EventLoop                *_acceptor_loop; // Acceptor的loop
  const std::string         _ip_port;
  const std::string         _name;
  std::unique_ptr<Acceptor> _acceptor; // 内含channel

  std::atomic<int32_t> _started{1};
  int64_t              _conn_id{0};
};
} // namespace muduo