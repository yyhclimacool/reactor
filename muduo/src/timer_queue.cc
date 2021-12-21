#include "timer_queue.h"

#include <glog/logging.h>
#include <sys/timerfd.h>

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
    , _timers()
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

TimerId TimerQueue::add_timer(Timestamp when, double interval, std::function<void()> timer_cb) {
  // TODO: use unique_ptr
  Timer *timer = new Timer(std::move(timer_cb), when, interval);
  _loop->run_in_loop(std::bind(&TimerQueue::add_timer_in_loop, this, timer));
  return TimerId(timer, timer->sequence());
}

void TimerQueue::add_timer_in_loop(Timer *timer) {
  _loop->assert_in_loop_thread();
  bool earliest_changed = insert(timer);
  if (earliest_changed) {
    reset_timerfd(_timerfd, timer->expiration());
  }
}

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

void TimerQueue::cancel(TimerId timerid) { _loop->run_in_loop(std::bind(&TimerQueue::cancel_in_loop, this, timerid)); }

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
  } else if (_calling_expired_timers) { // TODO: WHY?
    _canceling_timers.insert(timer);
  }
  assert(_timers.size() == _active_timers.size());
}

void TimerQueue::handle_read() {
  _loop->assert_in_loop_thread();
  auto now = Timestamp::now();
  read_timerfd(_timerfd, now);
  auto expired = get_expired(now);
  _calling_expired_timers = true;
  for (const auto &entry : expired) {
    entry.second->run();
  }
  _calling_expired_timers = false;
  reset(expired, now);
}

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

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now) {}