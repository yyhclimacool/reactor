#pragma once

#include <assert.h>

#include <atomic>
#include <boost/noncopyable.hpp>
#include <functional>
#include <memory>
#include <string>

#include "callbacks.h"
#include "event_loop_threadpool.h"

namespace muduo {

class EventLoop;
class InetAddress;
class Acceptor;

// 先实现TcpServer用来接受连接
class TcpServer : public boost::noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

public:
  TcpServer(EventLoop *acceptor_loop, const InetAddress &listen_addr, const std::string &name);
  ~TcpServer();

  void set_thread_num(int num_threads) {
    assert(0 <= num_threads);
    _thread_pool->set_thread_num(num_threads);
  }
  // not thread-safe
  void set_connection_callback(const ConnectionCallback &cb) { _connection_callback = cb; }
  // not thread-safe
  void set_message_callback(const MessageCallback &cb) { _message_callback = cb; }
  // not thread-safe
  void set_write_complete_callback(const WriteCompleteCallback &cb) { _write_complete_callback = cb; }
  void set_thread_init_callback(const ThreadInitCallback &cb) { _thread_init_callback = std::move(cb); }
  // valid after calling start
  std::shared_ptr<EventLoopThreadPool> thread_pool() { return _thread_pool; }
  // 调用多次start没问题，且是线程安全的
  void start();

  EventLoop         *get_loop() const { return _acceptor_loop; }
  const std::string  get_ip_port() const { return _ip_port; }
  const std::string &get_name() const { return _name; }

private:
  // 被Acceptor回调，非线程安全，在loop中调用是线程安全的
  void new_connection(int sockfd, const InetAddress &peer_addr);
  // thread-safe
  void remove_connection(const std::shared_ptr<TcpConnection> &conn);
  // not thread-safe, but in loop
  void remove_connection_in_loop(const std::shared_ptr<TcpConnection> &conn);

private:
  // Acceptor需要在一个loop上接受连接，Acceptor的handle_read会回调NewConnectionCallback
  EventLoop                           *_acceptor_loop;
  std::unique_ptr<Acceptor>            _acceptor; // 内含channel
  const std::string                    _ip_port;
  const std::string                    _name;
  std::shared_ptr<EventLoopThreadPool> _thread_pool;
  std::atomic<int32_t>                 _started{1};
  int64_t                              _conn_id{0};
  // TODO: Q: 如何保证对_connections的读写是线程安全的？
  std::map<std::string, std::shared_ptr<TcpConnection>> _connections;

  ConnectionCallback    _connection_callback;
  MessageCallback       _message_callback;
  WriteCompleteCallback _write_complete_callback;
  ThreadInitCallback    _thread_init_callback;
};

} // namespace muduo