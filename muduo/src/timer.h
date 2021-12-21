#pragma once

#include <atomic>
#include <functional>

#include "timestamp.h"

namespace muduo {

class Timer;
class TimerQueue;

class TimerId {
  friend class TimerQueue;

public:
  TimerId()
      : _timer(nullptr)
      , _sequence(0) {}
  TimerId(Timer *timer, int64_t seq)
      : _timer(timer)
      , _sequence(seq) {}

  Timer  *timer() const { return _timer; }
  int64_t sequence() const { return _sequence; }

private:
  Timer  *_timer;
  int64_t _sequence;
};

class Timer {
public:
  Timer(std::function<void()> cb, Timestamp when, double interval)
      : _callback(std::move(cb))
      , _expiration(when)
      , _interval(interval)
      , _repeat(interval > 0.0)
      , _sequence(++_s_num_created) {}

  void           run() const { _callback(); }
  Timestamp      expiration() const { return _expiration; }
  bool           repeat() const { return _repeat; }
  int64_t        sequence() const { return _sequence; }
  void           restart(Timestamp now);
  static int64_t num_created() { return _s_num_created; }

private:
  const std::function<void()> _callback;
  Timestamp                   _expiration;
  const double                _interval;
  const bool                  _repeat;
  const int64_t               _sequence;

  static std::atomic<int64_t> _s_num_created;
};

} // namespace muduo