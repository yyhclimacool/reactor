/* #include "muduo/src/event_loop.h"
#include "muduo/src/timer_queue.h"

#include <glog/logging.h>
#include <sys/timerfd.h>

#include "muduo/src/timer.h"

using namespace muduo;

int CreateTimerFd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        LOG(FATAL) << "create timerfd failed.";
    }
    return timerfd;
}

void ResetTimerFd(int timerfd, Timestamp expiration) {
    struct itimerspec new_value;
    struct itimerspec old_value;
    memset(&new_value, sizeof(new_value), 0x00);
    memset(&old_value, sizeof(old_value), 0x00);
    int64_t ms = expiration - Timestamp::Now();
    if (ms < 100) ms = 100;
    new_value.it_value.tv_sec = static_cast<time_t>(ms / Timestamp::kMicroSecondsPerSecond);
    new_value.it_value.tv_nsec = static_cast<long>((ms % Timestamp::kMicroSecondsPerSecond) * 1000);
    int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
    if (ret != 0) {
        LOG(FATAL) << "::timerfd_settime failed, when=" << expiration;
    }
}

TimerQueue::TimerQueue(EventLoop *loop)
    :  _loop(loop),
       _timerfd(CreateTimerFd()),
       _timerfd_channel(loop, _timerfd),
       _timers(),
       _calling_expired_timers(false) {

    _timerfd_channel.SetReadCallback(std::bind(&TimerQueue::HandleRead, this));

    // we are always reading the timerfd, we disarm it with timerfd_settime
    _timerfd_channel.EnableReading();
}

TimerId TimerQueue::AddTimer(Timestamp when, double interval, std::function<void()> timer_cb) {
    // TODO: check where to delete, or use unique_ptr
    Timer *timer = new Timer(std::move(timer_cb), when, interval);
    _loop->RunInLoop(std::bind(&TimerQueue::AddTimerInLoop, this, timer));
    return TimerId(timer, timer->Sequence());
}

void TimerQueue::AddTimerInLoop(Timer *timer) {
    _loop->AssertInLoopThread();
    bool earliest_changed = Insert(timer);
    if (earliest_changed) {
        ResetTimerFd(_timerfd, timer->Expiration());
    }
}

bool TimerQueue::Insert(Timer *timer) {
    _loop->AssertInLoopThread();
    assert(_timers.size() == _active_timers.size());
    bool earliest_changed = false;
    Timestamp when = timer->Expiration();
    auto it = _timers.begin();
    if (it == _timers.end() || when < it->first) {
        earliest_changed = true;
    }
    {
        auto result = _timers.insert(std::make_pair(when, timer));
        assert(result.second);
        (void) result;
    }
    {
        auto result = _active_timers.insert({timer, timer->Sequence()});
        assert(result.second);
        (void) result;
    }
    assert(_timers.size() == _active_timers.end());
    return earliest_changed;
}

void TimerQueue::HandleRead() {
//    _loop->AssertInLoopThread();
//    Timestamp now(Timestamp::Now());
//
//    ReadTimerFd(_timerfd, now);
}
 */