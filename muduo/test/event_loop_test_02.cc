#include <thread>

#include "muduo/src/event_loop.h"

using namespace muduo;

EventLoop *g_loop;

void thread_func() { g_loop->loop(); }

int main() {
  EventLoop loop;
  g_loop = &loop;
  std::thread bgt(thread_func);
  bgt.join();

  return 0;
}
