#include "event_loop_thread.h"

#include <glog/logging.h>

#include "event_loop.h"

using namespace muduo;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : _loop(nullptr)
    , _exiting(false)
    , _thread(std::bind(&EventLoopThread::thread_func, this))
    , _mutex()
    , _cond()
    , _callback(cb)
    , _name(name) {
  DLOG(INFO) << "constructed an EventLoopThread, name[" << _name << "], this[" << this << "], thread["
             << _thread.get_id() << "]";
}

EventLoopThread::~EventLoopThread() {
  _exiting = true;
  // not 100% thread-safe, eg. thread_func could be running _callback
  // 在析构阶段强制等待_loop创建好，否则会core dump
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [ this ] { return _loop != nullptr; });
  }
  if (_loop) {
    DLOG(INFO) << "In destructor of EventLoopThread[" << _name << "], loop[" << _loop << "], quit the loop";
    _loop->quit();
  }
  if (_thread.joinable()) {
    DLOG(INFO) << "In destructor of EventLoopThread[" << _name << "], join the thread[" << _thread.get_id() << "]";
    _thread.join();
  }
}

// 线程入口函数，loop是栈上对象
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

  // Q: 通常loop是一个无限循环，什么时候会退出loop？
  // A: _loop指针会返回给调用者，因而调用者可以保存指针，并在合适的时候调用quit()退出loop
  //    this对象析构的时候也会调用quit。
  loop.loop();

  // exiting ?
  {
    std::lock_guard<std::mutex> lg(_mutex);
    _loop = nullptr;
  }
}

// 在条件变量上等待，直到创建了loop，返回loop在栈上的地址
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