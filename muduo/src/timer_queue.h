#pragma once

class EventLoop;
class TimerId;

// A best effort timer queue.
// No guarantee that the callback will be on time.
class TimerQueue {
public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    TimerId AddTimer(std::function<void()> timer_cb;
                     Timestamp when,
                     double interval);
    void Cancel(TimerId timerid);

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
    bool _calling_expired_timers; /* atomic */
    ActiveTimerSet _canceling_timers;
};
