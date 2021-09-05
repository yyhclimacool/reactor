#include "muduo/src/event_loop.h"
#include "muduo/src/current_thread.h"
#include "muduo/src/channel.h"

#include <sys/timerfd.h>
#include <glog/logging.h>
#include <functional>
#include <utility>

using namespace muduo;

EventLoop *g_loop;
int timerfd;

void timeout(Timestamp receive_time, Channel *ch) {
    LOG(INFO) << "Timeout !" << " receive_time=" << receive_time.ToFormattedString() << ", pid=" << getpid() << ", tid=" << tid();
    uint64_t howmany;
    // LT
    read(timerfd, &howmany, sizeof(howmany));
    g_loop->Quit();

    ch->DisableAll();
    ch->Remove();
}

int main() {
    EventLoop loop;
    g_loop = &loop;

    timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    Channel timer_channel(&loop, timerfd);
    timer_channel.SetReadCallback(std::bind(timeout, std::placeholders::_1, &timer_channel));
    timer_channel.EnableReading();

    struct itimerspec howlong;
    memset(&howlong, 0x00, sizeof howlong);
    howlong.it_value.tv_sec = 1;
    timerfd_settime(timerfd, 0, &howlong, NULL);

    loop.Loop();

    //timer_channel.DisableAll();
    //timer_channel.Remove();

    close(timerfd);
}
