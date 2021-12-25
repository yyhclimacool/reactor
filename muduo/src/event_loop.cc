#include "event_loop.h"

#include <glog/logging.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "channel.h"
#include "current_thread.h"
#include "poller.h"
#include "timer.h"
#include "timer_queue.h"

int create_eventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG(FATAL) << "create eventfd for event_loop failed.";
  }
  return evtfd;
}

using namespace muduo;

thread_local EventLoop *t_LoopInThisThread = nullptr;

EventLoop::EventLoop()
    : _poller(Poller::new_default_poller(this))
    , _threadid(tid())
    , _iteration(0)
    , _looping(false)
    , _quit(false)
    , _wakeupfd(create_eventfd())
    , _wakeup_channel(new Channel(this, _wakeupfd))
    , _timer_queue(new TimerQueue(this)) {
  // 如果当前线程的EventLoop已经存在，严重错误，退出程序
  if (t_LoopInThisThread) {
    LOG(FATAL) << "another eventloop=" << t_LoopInThisThread << " exists in current thread=" << tid();
  } else {
    t_LoopInThisThread = this;
  }
  _wakeup_channel->set_read_callback(std::bind(&EventLoop::handle_read, this));
  _wakeup_channel->enable_reading();
  DLOG(INFO) << "create an event_loop=" << this << " in thread=" << _threadid;
}

// TODO: 可以跨线程调用吗？貌似也不行，因为不同的线程有不同的线程局部存储: t_LoopInThisThread
EventLoop::~EventLoop() {
  DLOG(INFO) << "eventloop=" << this << " of thread=" << _threadid << " destructs in thread=" << tid();
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
  LOG(INFO) << "event_loop[" << this << "] start looping ...";
  while (!_quit) {
    DLOG(INFO) << "event_loop[" << this << "] one round of loopping ...";
    _active_channels.clear();
    _poll_return_ts = _poller->poll(10000 /*ms*/, &_active_channels);
    _event_handling = true;
    // TODO: sort channel by priority
    for (auto it = _active_channels.begin(); it != _active_channels.end(); ++it) {
      _current_active_channel = *it;
      _current_active_channel->handle_event(_poll_return_ts);
    }
    _current_active_channel = nullptr;
    _event_handling = false;
    call_pending_functors();
  }
  LOG(INFO) << "event_loop[" << this << "] stopped looping.";
  _looping = false;
}

// 可以在非I/O线程调用
void EventLoop::quit() {
  _quit = true;
  if (!is_in_loop_thread()) {
    // TODO: wakeup event loop
    wakeup();
  }
  DLOG(INFO) << "set quit flag of event_loop=" << this << ", pid=" << getpid() << ", tid=" << tid()
             << ", this loop's owner tid=" << _threadid;
  LOG(INFO) << "event_loop[" << this << "] asked to quit.";
}

void EventLoop::call_pending_functors() {
  std::vector<std::function<void()>> functors;
  _calling_pending_functors = true;
  {
    std::lock_guard<std::mutex> gd(_mutex);
    functors.swap(_pending_functors);
  }
  for (const auto &func : functors) {
    func();
  }
  _calling_pending_functors = false;
}

// TODO: 这些func会有多及时被调用？
void EventLoop::run_in_loop(std::function<void()> func) {
  if (is_in_loop_thread()) {
    func();
  } else {
    queue_in_loop(std::move(func));
  }
}

void EventLoop::queue_in_loop(std::function<void()> func) {
  {
    std::lock_guard<std::mutex> guard(_mutex);
    _pending_functors.push_back(std::move(func));
  }
  // TODO: What to do with _calling_pending_functors ?
  // 为了让func被及时的调用
  // 如果I/O线程阻塞在poll上，那么需要立刻唤醒
  // 如果I/O线程已经处理完了epoll的事件，正在调用pending functors，为什么需要唤醒？唤醒如何起作用？
  // 后一种情况下，func可能会被挂起很久！
  if (!is_in_loop_thread() || _calling_pending_functors) {
    wakeup();
  }
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t  n = write(_wakeupfd, &one, sizeof one);
  if (n != sizeof one) {
    LOG(WARNING) << "EventLoop::wakeup write " << n << " bytes instead of " << sizeof one << " bytes.";
  }
}

// wakeup channel 的 read callback
void EventLoop::handle_read() {
  uint64_t one = 1;
  ssize_t  n = read(_wakeupfd, &one, sizeof one);
  if (n != sizeof one) {
    LOG(WARNING) << "EventLoop::handle_read read " << n << " bytes instead of " << sizeof one << " bytes.";
  }
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

TimerId EventLoop::run_at(Timestamp time, std::function<void()> cb) {
  return _timer_queue->add_timer(time, 0.0, std::move(cb));
}

TimerId EventLoop::run_after(double delay, std::function<void()> cb) {
  auto time = Timestamp::now() + delay;
  return run_at(time, cb);
}

TimerId EventLoop::run_every(double interval, std::function<void()> cb) {
  auto time = Timestamp::now() + interval;
  return _timer_queue->add_timer(time, interval, std::move(cb));
}

void EventLoop::cancel(TimerId timerid) { return _timer_queue->cancel(timerid); }
