#include "event_loop_thread.h"

#include "event_loop.h"

using namespace muduo;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : _loop(nullptr)
    , _exiting(false)
    , _thread(std::bind(&EventLoopThread::thread_func, this))
    , _mutex()
    , _cond()
    , _callback(cb)
    , _name(name) {}

EventLoopThread::~EventLoopThread() {
  _exiting = true;
  // not 100% thread-safe, eg. thread_func could be running _callback
  if (_loop) {
    _loop->quit();
    _thread.join();
  }
}

void EventLoopThread::thread_func() {
  EventLoop loop;
  if (_callback) {
    _callback(&loop);
  }
  {
    std::lock_guard<std::mutex> lg(_mutex);
    _loop = &loop;
    _cond.notify_one();
  }
  loop.loop();

  // exiting ?
  std::lock_guard<std::mutex> lg(_mutex);
  _loop = nullptr;
}

EventLoop *EventLoopThread::start_loop() {
  assert(_thread.joinable());
  EventLoop *loop = nullptr;
  {
    std::unique_lock<std::mutex> ul(_mutex);
    _cond.wait(ul, [ this ]() { return _loop != nullptr; });
    loop = _loop;
  }
  return loop;
}