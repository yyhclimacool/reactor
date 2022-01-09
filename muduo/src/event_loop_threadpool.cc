#include "muduo/src/event_loop_threadpool.h"

#include "muduo/src/event_loop.h"
#include "muduo/src/event_loop_thread.h"

namespace muduo {

EventLoopThreadPool::EventLoopThreadPool(EventLoop *base_loop, const std::string &name)
    : _base_loop(base_loop)
    , _name(name)
    , _started(false)
    , _num_threads(0)
    , _next(0)
    , _threads()
    , _loops() {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
  assert(!_started);
  _base_loop->assert_in_loop_thread();
  _started = true;
  for (int i = 0; i < _num_threads; ++i) {
    char buf[ _name.size() + 32 ];
    snprintf(buf, sizeof buf, "%s%d", _name.c_str(), i);
    _threads.emplace_back(new EventLoopThread(cb, buf));
    _loops.push_back(_threads.back()->start_loop());
  }

  // TODO: 这一行什么作用？
  if (_num_threads == 0 && cb) {
    cb(_base_loop);
  }
}

EventLoop *EventLoopThreadPool::get_next_loop() {
  _base_loop->assert_in_loop_thread();
  assert(_started);
  EventLoop *loop = _base_loop;
  if (!_loops.empty()) {
    // round-robin
    loop = _loops[ _next ];
    ++_next;
    if (static_cast<size_t>(_next) >= _loops.size()) {
      _next = 0;
    }
  }
  return loop;
}

EventLoop *EventLoopThreadPool::get_loop_for_hash(size_t hash_code) {
  _base_loop->assert_in_loop_thread();
  EventLoop *loop = _base_loop;
  if (!_loops.empty()) {
    loop = _loops[ hash_code % _loops.size() ];
  }
  return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::get_all_loops() {
  _base_loop->assert_in_loop_thread();
  assert(_started);
  if (_loops.empty()) {
    return std::vector<EventLoop *>(1, _base_loop);
  } else {
    return _loops;
  }
}

} // namespace muduo
