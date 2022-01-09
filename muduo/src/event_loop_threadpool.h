#pragma once

#include <boost/noncopyable.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace muduo {
class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : public boost::noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThreadPool(EventLoop *base_loop, const std::string &name);
  ~EventLoopThreadPool();

  void set_thread_num(int num_threads) { _num_threads = num_threads; }
  void start(const ThreadInitCallback &cb = ThreadInitCallback());

  EventLoop               *get_next_loop();
  EventLoop               *get_loop_for_hash(size_t hash_code);
  std::vector<EventLoop *> get_all_loops();
  bool                     started() const { return _started; }
  const std::string       &name() const { return _name; }

private:
  EventLoop                                    *_base_loop;
  std::string                                   _name;
  bool                                          _started;
  int                                           _num_threads;
  int                                           _next;
  std::vector<std::unique_ptr<EventLoopThread>> _threads;
  std::vector<EventLoop *>                      _loops;
};
} // namespace muduo