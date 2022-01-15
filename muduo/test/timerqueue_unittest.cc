#include <glog/logging.h>
#include <stdio.h>
#include <unistd.h>

#include <thread>

#include "muduo/src/current_thread.h"
#include "muduo/src/event_loop.h"

using namespace muduo;

int        cnt = 0;
EventLoop *g_loop;

void print_tid() {
  LOG(INFO) << "pid[" << getpid() << "], tid[" << tid() << "].";
  LOG(INFO) << "now[" << Timestamp::now().to_string() << "].";
}

void print(const char *msg) {
  LOG(INFO) << "msg[" << msg << "], now[" << Timestamp::now().to_string() << "].";
  if (++cnt == 20) {
    g_loop->quit();
  }
}

void timer_callback_with_cancel(const char *msg, TimerId timerid) {
  LOG(INFO) << "msg[" << msg << "], now[" << Timestamp::now().to_string() << "].";
  g_loop->cancel(timerid);
}

void cancel(TimerId timer) {
  g_loop->cancel(timer);
  LOG(INFO) << "cancelled at " << Timestamp::now().to_string() << ".";
}

void test_thread() {
  EventLoop &loop = *g_loop;
  /*   loop.run_after(1, std::bind(print, "1s passed"));
    loop.run_after(1.5, std::bind(print, "another 1.5s passed"));
    loop.run_after(2.5, std::bind(print, "another 2.5s passed"));
    loop.run_after(3.5, std::bind(print, "another 3.5s passed"));

    // 4.5s later, run print
    TimerId t45 = loop.run_after(4.5, std::bind(print, "another 4.5s passed"));
    // 4.2s later, run cancel, so message above will not be printed
    loop.run_after(4.2, std::bind(cancel, t45));
    // TODO: what happens if call cancel one more time?
    // cancel 2次没有什么伤害
    loop.run_after(4.8, std::bind(cancel, t45)); */

  auto timerid = loop.run_every(1.1, std::bind(print, "show me every 1.1s"));
  loop.run_after(16, std::bind(&EventLoop::cancel, g_loop, timerid));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  // google::InitGoogleLogging(argv[ 0 ]);
  print_tid();
  sleep(1);
  {
    EventLoop loop;
    g_loop = &loop;

    print("main"); // this time not calling g_loop->quit()
    std::thread t{test_thread};
    t.detach();
    loop.loop();
  }
}