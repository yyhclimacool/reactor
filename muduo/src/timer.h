/* #pragma once

#include <atomic>
#include <functional>

#include "muduo/src/timestamp.h"

namespace muduo {

class Timer;
class TimerQueue;

class TimerId {
    friend class TimerQueue;
public:
    TimerId() : _timer(nullptr), _sequence(0) {}
    TimerId(Timer *timer, int64_t seq) : _timer(timer), _sequence(seq) {}
private:
    Timer *_timer;
    int64_t _sequence;
};

class Timer {
public:
    Timer(std::function<void()> cb,
          Timestamp when,
          double interval)
        : _callback(std::move(cb)),
          _expiration(when),
          _interval(interval),
          _repeat(interval > 0.0),
          _sequence(++_s_num_created) { }

    void Run() const { _callback(); }
    Timestamp Expiration() const { return _expiration; }
    bool Repeat() const { return _repeat; }
    int64_t Sequence() const { return _sequence; }
    void Restart(Timestamp now);
    static int64_t NumCreated() { return _s_num_created; }
private:
    const std::function<void()> _callback;
    Timestamp _expiration;
    const double _interval;
    const bool _repeat;
    const int64_t _sequence;

    static std::atomic<int64_t> _s_num_created;
};

} // namespace muduo
 */