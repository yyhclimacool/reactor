#pragma once

#include <assert.h>

#include <atomic>
#include <boost/noncopyable.hpp>
#include <functional>
#include <memory>
#include <string>

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
  void set_thread_init_callback(const ThreadInitCallback &cb) { _thread_init_callback = std::move(cb); }
  // valid after calling start
  std::shared_ptr<EventLoopThreadPool> thread_pool() { return _thread_pool; }
  // 调用多次start没问题，且是线程安全的
  void start();

  const std::string  ip_port() const { return _ip_port; }
  const std::string &name() const { return _name; }
  EventLoop         *get_loop() const { return _acceptor_loop; }

private:
  // 被Acceptor回调
  void new_connection(int sockfd, const InetAddress &peer_addr);

private:
  // Acceptor需要在一个loop上接受连接，Acceptor的handle_read会回调NewConnectionCallback
  EventLoop                           *_acceptor_loop;
  std::unique_ptr<Acceptor>            _acceptor; // 内含channel
  const std::string                    _ip_port;
  const std::string                    _name;
  std::shared_ptr<EventLoopThreadPool> _thread_pool;
  ThreadInitCallback                   _thread_init_callback;
  std::atomic<int32_t>                 _started{1};
  int64_t                              _conn_id{0};
};

} // namespace muduo