#include "event_loop.h"

#include <glog/logging.h>

#include "channel.h"
#include "current_thread.h"
#include "poller.h"
#include "timer.h"
#include "timer_queue.h"

using namespace muduo;

thread_local EventLoop *t_LoopInThisThread = nullptr;

EventLoop::EventLoop()
    : _poller(Poller::new_default_poller(this))
    , _threadid(tid())
    , _iteration(0)
    , _looping(false)
    , _quit(false)
/* , _timer_queue(new TimerQueue(this)) */ {
  // 如果当前线程的EventLoop已经存在，严重错误，退出程序
  if (t_LoopInThisThread) {
    LOG(FATAL) << "another eventloop=" << t_LoopInThisThread << " exists in current thread=" << tid();
  } else {
    t_LoopInThisThread = this;
  }
  LOG(INFO) << "create an event_loop=" << this << " in thread=" << _threadid;
}

// TODO: 可以跨线程调用吗？貌似也不行，因为不同的线程有不同的线程局部存储: t_LoopInThisThread
EventLoop::~EventLoop() {
  LOG(INFO) << "eventloop=" << this << " of thread=" << _threadid << " destructs in thread=" << tid();
  // 置空
  t_LoopInThisThread = nullptr;
}

EventLoop *EventLoop::event_loop_of_current_thread() { return t_LoopInThisThread; }

// 通过判断当前的线程id和EventLoop对象创建时的线程id
bool EventLoop::is_in_loop_thread() const { return _threadid == tid(); }

void EventLoop::assert_in_loop_thread() const {
  if (!is_in_loop_thread()) {
    LOG(FATAL) << "assert_in_loop_thread failed. abort program. event_loop=" << this
               << ", created thread_id=" << _threadid << ", current threadid=" << tid();
  }
}

// 只能在创建对象的线程中调用
// 该线程称作I/O线程
void EventLoop::loop() {
  assert(!_looping);
  assert_in_loop_thread();
  _looping = true;
  LOG(INFO) << "event_loop=" << this << " start looping ...";
  while (!_quit) {
    _active_channels.clear();
    _poll_return_time = _poller->poll(1000 /*ms*/, &_active_channels);
    _event_handling = true;
    // TODO: sort channel by priority
    for (auto it = _active_channels.begin(); it != _active_channels.end(); ++it) {
      _current_active_channel = *it;
      _current_active_channel->handle_event(_poll_return_time);
    }
    _current_active_channel = nullptr;
    _event_handling = false;
  }
  LOG(INFO) << "event_loop=" << this << " stop looping.";
  _looping = false;
}

// 可以在非I/O线程调用
void EventLoop::quit() {
  _quit = true;
  if (!is_in_loop_thread()) {
    // TODO: wakeup event loop
    // wakeup();
  }
  LOG(INFO) << "set quit flag of event_loop=" << this << ", pid=" << getpid() << ", tid=" << tid()
            << ", this loop's owner tid=" << _threadid;
}

// 可以跨线程调用吗？不可以！
bool EventLoop::has_channel(Channel *ch) {
  assert(ch->owner_loop() == this);
  assert_in_loop_thread();
  return _poller->has_channel(ch);
}

// 也不能跨线程调用
void EventLoop::remove_channel(Channel *ch) {
  assert(ch->owner_loop() == this);
  assert_in_loop_thread();
  // TODO: 如果处于_event_handling中，会是什么情况？
  if (_event_handling) {
    assert(_current_active_channel == ch ||
           std::find(_active_channels.begin(), _active_channels.end(), ch) == _active_channels.end());
  }
  // 在_poller中执行实际的remove_channel
  _poller->remove_channel(ch);
}

void EventLoop::update_channel(Channel *ch) {
  assert(ch->owner_loop() == this);
  assert_in_loop_thread();
  // 在_poller中执行实际的update_channel
  _poller->update_channel(ch);
}

// TimerId EventLoop::RunAt(Timestamp time, std::function<void()> cb) {
//     return _timer_queue->AddTimer(time, 0.0, std::move(cb));
// }

// TimerId EventLoop::RunAfter(double delay, std::function<void()> cb) {
//     auto time = Timestamp::Now() + delay;
//     return RunAt(time, cb);
// }

// TimerId EventLoop::RunEvery(double interval, std::function<void()> cb) {
//     auto time = Timestamp::Now() + interval;
//     return _timer_queue->AddTimer(time, interval, std::move(cb));
// }

// void EventLoop::Cancel(TimerId timerid) {
//     return _timer_queue->Cancel(timerid);
// }
