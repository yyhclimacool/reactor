#include "timer_queue.h"

#include <glog/logging.h>
#include <sys/timerfd.h>

#include "current_thread.h"
#include "event_loop.h"
#include "timer.h"

using namespace muduo;

int create_timerfd() {
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0) {
    LOG(FATAL) << "create timerfd failed.";
  }
  return timerfd;
}

void read_timerfd(int timerfd, Timestamp now) {
  uint64_t howmany;
  ssize_t  n = ::read(timerfd, &howmany, sizeof howmany);
  LOG(INFO) << "read timerfd in TimerQueue::handle_read, data[" << howmany << "] at ts[" << now.to_string() << "].";
  if (n != sizeof howmany) {
    LOG(WARNING) << "read timerfd in TimerQueue::handle_read, read size[" << n << "] instead of 8";
  }
}

// 被TimerQueue::insert在loop线程中执行
void reset_timerfd(int timerfd, Timestamp expiration) {
  struct itimerspec new_value;
  struct itimerspec old_value;
  memset(&new_value, 0x00, sizeof(new_value));
  memset(&old_value, 0x00, sizeof(old_value));
  int64_t ms = expiration - Timestamp::now();
  if (ms < 100) ms = 100; // micro-seconds not milli-seconds
  new_value.it_value.tv_sec = static_cast<time_t>(ms / Timestamp::kMicroSecondsPerSecond);
  new_value.it_value.tv_nsec = static_cast<uint64_t>((ms % Timestamp::kMicroSecondsPerSecond) * 1000);
  int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
  if (ret != 0) {
    LOG(FATAL) << "::timerfd_settime failed, when=" << expiration;
  }
}

TimerQueue::TimerQueue(EventLoop *loop)
    : _loop(loop)
    , _timerfd(create_timerfd())
    , _timerfd_channel(loop, _timerfd)
    , _calling_expired_timers(false) {
  _timerfd_channel.set_read_callback(std::bind(&TimerQueue::handle_read, this));
  // we are always reading the timerfd, we disalarm it with timerfd_settime
  _timerfd_channel.enable_reading();
}

TimerQueue::~TimerQueue() {
  _timerfd_channel.disable_all();
  _timerfd_channel.remove();
  ::close(_timerfd);
  for (auto &entry : _timers) {
    delete entry.second;
  }
}

// 添加定时器需要告诉我【何时触发】【定时器的重复间隔】和【定时器触发时的回调】
TimerId TimerQueue::add_timer(Timestamp when, double interval, std::function<void()> timer_cb) {
  // TODO: use unique_ptr
  Timer *timer = new Timer(std::move(timer_cb), when, interval);
  // 在loop线程中执行真正的add_timer操作
  _loop->run_in_loop(std::bind(&TimerQueue::add_timer_in_loop, this, timer));
  return TimerId(timer, timer->sequence());
}

// 在loop线程中执行真正的add_timer操作
void TimerQueue::add_timer_in_loop(Timer *timer) {
  DLOG(INFO) << "call " << __func__ << " in loop thread";
  _loop->assert_in_loop_thread();
  bool earliest_changed = insert(timer);
  if (earliest_changed) {
    reset_timerfd(_timerfd, timer->expiration());
  }
}

// 在loop线程中执行
// @return: 新加入的这个timer是否是最早的触发超时的timer，超时时间距离现在最近
bool TimerQueue::insert(Timer *timer) {
  _loop->assert_in_loop_thread();
  assert(_timers.size() == _active_timers.size());
  bool      earliest_changed = false;
  Timestamp when = timer->expiration();
  auto      it = _timers.begin();
  if (it == _timers.end() || when < it->first) {
    earliest_changed = true;
  }
  {
    auto result = _timers.insert(std::make_pair(when, timer));
    assert(result.second);
    (void)result;
  }
  {
    auto result = _active_timers.insert({timer, timer->sequence()});
    assert(result.second);
    (void)result;
  }
  assert(_timers.size() == _active_timers.size());
  return earliest_changed;
}

// 非loop线程中调用
void TimerQueue::cancel(TimerId timerid) { _loop->run_in_loop(std::bind(&TimerQueue::cancel_in_loop, this, timerid)); }

// 在loop线程中执行，这个函数有可能在TimerQueue::handle_read中被调用
void TimerQueue::cancel_in_loop(TimerId timerid) {
  _loop->assert_in_loop_thread();
  assert(_timers.size() == _active_timers.size());
  ActiveTimer timer(timerid.timer(), timerid.sequence());
  auto        it = _active_timers.find(timer);
  if (it != _active_timers.end()) {
    size_t n = _timers.erase({it->first->expiration(), it->first});
    assert(n == 1);
    (void)n;
    delete it->first; // FIXME: no delete
    _active_timers.erase(it);
  } else if (_calling_expired_timers) {
    // TODO: WHY?
    // 一方面，这个函数在TimerQueue::handle_read中被调用
    // 另一方面，如果同一个timerid被cancel了2次，就把它放入_calceling_timers
    _canceling_timers.insert(timer);
  }
  assert(_timers.size() == _active_timers.size());
}

// _timerfd的超时时间到了，就调用handle_read，在I/O线程中调用
// TODO: 如何让本函数在loop线程中执行？通过channel的handle_read
void TimerQueue::handle_read() {
  _loop->assert_in_loop_thread();
  auto now = Timestamp::now();
  read_timerfd(_timerfd, now);
  auto expired = get_expired(now);
  _calling_expired_timers = true;
  _canceling_timers.clear();
  for (const auto &entry : expired) {
    entry.second->run();
  }
  _calling_expired_timers = false;
  reset(expired, now);
}

// 获取所有到期时间早于现在的timers，就是已经到期的timers
// 将这些timers从_timers和_active_timers删除
// 在loop线程中执行
std::vector<TimerQueue::Entry> TimerQueue::get_expired(Timestamp now) {
  assert(_timers.size() == _active_timers.size());
  std::vector<Entry> expired;
  Entry              sentry = std::make_pair(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
  auto               it = _timers.lower_bound(sentry);
  assert(it == _timers.end() || now < it->first);
  std::copy(_timers.begin(), it, back_inserter(expired));
  _timers.erase(_timers.begin(), it);

  for (const auto &entry : expired) {
    ActiveTimer timer{entry.second, entry.second->sequence()};
    auto        ret = _active_timers.erase(timer);
    assert(ret == 1);
    (void)ret;
  }

  assert(_timers.size() == _active_timers.size());

  return expired;
}

// 在loop线程中执行
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now) {
  Timestamp next_expire;
  for (const Entry &elem : expired) {
    ActiveTimer timer(elem.second, elem.second->sequence());

    // TODO: 这里的_canceling_timers什么作用？
    // 这个已经到期的timer处理完了，并且对该timer执行了cancel了，那就意味着不需要在使用这个timer了，也不需要reset，再insert了
    // 所以在timer到期的回调里，可以直接执行cancel
    if (elem.second->repeat() && _canceling_timers.find(timer) == _canceling_timers.end()) {
      elem.second->restart(now);
      insert(elem.second);
    } else {
      delete elem.second;
    }
  }

  if (!_timers.empty()) {
    next_expire = _timers.begin()->second->expiration();
  }

  if (next_expire.valid()) {
    reset_timerfd(_timerfd, next_expire);
  }
}