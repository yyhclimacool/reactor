#include "muduo/src/timer.h"

std::atomic<int64_t> Timer::_s_num_created;

void Timer::Restart(Timestamp now) {
    if (_repeat) {
        now += _interval;
        _expiration = now;
    } else {
        _expiration = Timestamp::Invalid();
    }
}
