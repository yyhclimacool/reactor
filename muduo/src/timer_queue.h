#pragma once

#include <atomic>
#include <functional>
#include <set>

#include "channel.h"
#include "timestamp.h"

namespace muduo {

class EventLoop;
class TimerId;
class Timer;

// A best effort timer queue.
// No guarantee that the callback will be on time.
// TimerQueue所属的成员函数只能在其所属的I/O线程中调用，因此不必加锁
class TimerQueue {
public:
  typedef std::pair<Timestamp, Timer *> Entry;
  typedef std::set<Entry>               TimerSet;
  typedef std::pair<Timer *, int64_t>   ActiveTimer;
  typedef std::set<ActiveTimer>         ActiveTimerSet;

public:
  explicit TimerQueue(EventLoop *loop);
  ~TimerQueue();

  TimerId add_timer(Timestamp when, double interval, std::function<void()> timer_cb);
  void    cancel(TimerId timerid);

private:
  void add_timer_in_loop(Timer *timer);
  bool insert(Timer *);
  void cancel_in_loop(TimerId timerid);

  // called when timerfd alarms
  void handle_read();
  // move out all expired timers
  std::vector<Entry> get_expired(Timestamp now);
  void               reset(const std::vector<Entry> &expired, Timestamp now);

private:
  EventLoop *_loop;
  const int  _timerfd;
  Channel    _timerfd_channel;
  TimerSet   _timers;

  ActiveTimerSet    _active_timers;
  std::atomic<bool> _calling_expired_timers;
  ActiveTimerSet    _canceling_timers;
};

} // namespace muduo