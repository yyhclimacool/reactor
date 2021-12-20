#include <glog/logging.h>
#include <sys/timerfd.h>

#include <functional>
#include <utility>

#include "muduo/src/channel.h"
#include "muduo/src/current_thread.h"
#include "muduo/src/event_loop.h"

using namespace muduo;

EventLoop *g_loop;
int        timerfd;

void timeout(Timestamp receive_time, Channel *ch) {
  LOG(INFO) << "Timeout !"
            << " receive_time=" << receive_time.to_formatted_string() << ", pid=" << getpid() << ", tid=" << tid();
  // LT
  uint64_t howmany;
  read(timerfd, &howmany, sizeof(howmany));
  g_loop->quit();

  ch->disable_all();
  ch->remove();
}

int main() {
  EventLoop loop;
  g_loop = &loop;

  timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

  Channel timer_channel(&loop, timerfd);
  timer_channel.set_read_callback(std::bind(timeout, std::placeholders::_1, &timer_channel));
  timer_channel.enable_reading();

  struct itimerspec howlong;
  memset(&howlong, 0x00, sizeof howlong);
  howlong.it_value.tv_sec = 1;
  timerfd_settime(timerfd, 0, &howlong, NULL);

  loop.loop();

  close(timerfd);
}
