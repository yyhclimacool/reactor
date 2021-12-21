#include "timer.h"

using namespace muduo;

std::atomic<int64_t> Timer::_s_num_created;

void Timer::restart(Timestamp now) {
  if (_repeat) {
    now += _interval;
    _expiration = now;
  } else {
    // 非repeat定时器不能restart
    _expiration = Timestamp::invalid();
  }
}
