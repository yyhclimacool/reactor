#pragma once

#include <boost/noncopyable.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace muduo {

class EventLoop;

// 运行EventLoop的线程，EventLoop对象和该线程一一对应
// 主要流程总结如下：
// 1. 在线程入口函数中创建EventLoop对象，是一个栈上对象，先通知等待线程，再运行loop函数。
// 2. 通过条件变量通知等待线程，等待线程在条件变量上等待该EventLoop对象被创建，返回EventLoop对象在栈上的地址
class EventLoopThread : public boost::noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;
  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                  const std::string        &name = "default_eventloop_thread");
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