#pragma once

#include "muduo/src/channel.h"
#include "muduo/src/timestamp.h"

#include <atomic>
#include <functional>
#include <set>

namespace muduo {

class EventLoop;
class TimerId;
class Timer;

// A best effort timer queue.
// No guarantee that the callback will be on time.
class TimerQueue {
public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    TimerId AddTimer(Timestamp when, double interval, std::function<void()> timer_cb);
    void Cancel(TimerId timerid);

private:
    void HandleRead();
    void AddTimerInLoop(Timer *timer);
    bool Insert(Timer *);

private:
    typedef std::pair<Timestamp, Timer *> Entry;
    typedef std::set<Entry> TimerSet;
    typedef std::pair<Timer *, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    EventLoop *_loop;
    const int _timerfd;
    Channel _timerfd_channel;
    TimerSet _timers;

    ActiveTimerSet _active_timers;
    std::atomic<bool> _calling_expired_timers;
    ActiveTimerSet _canceling_timers;
};

} // namespace muduo
