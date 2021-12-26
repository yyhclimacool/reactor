#pragma once

#include <boost/noncopyable.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace muduo {
class EventLoop;
class EventLoopThread : public boost::noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;
  EventLoopThread(const ThreadInitCallback &cb, const std::string &name = "default_eventloop_thread");
  ~EventLoopThread();

  EventLoop *start_loop();

private:
  void                    thread_func();
  EventLoop              *_loop; // guarded by mutex
  bool                    _exiting;
  std::thread             _thread;
  std::mutex              _mutex;
  std::condition_variable _cond; // guarded by _mutex
  ThreadInitCallback      _callback;
  std::string             _name;
};
} // namespace muduo