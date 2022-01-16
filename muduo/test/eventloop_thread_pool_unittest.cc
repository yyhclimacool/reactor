
#include <glog/logging.h>

#include "muduo/src/current_thread.h"
#include "muduo/src/event_loop.h"
#include "muduo/src/event_loop_threadpool.h"

using namespace muduo;

void print(EventLoop *loop = nullptr) {
  LOG(INFO) << "print: pid[" << getpid() << "], tid[" << tid() << "], loop[" << loop << "]";
}

void init(EventLoop *loop) { LOG(INFO) << "init: pid[" << getpid() << "], tid[" << tid() << "], loop[" << loop << "]"; }

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  print();

  EventLoop loop;
  loop.run_after(11, std::bind(&EventLoop::quit, &loop));
  {
    LOG(INFO) << "Single thread";
    EventLoopThreadPool model(&loop, "single");
    model.set_thread_num(0);
    model.start(init);
    assert(model.get_next_loop() == &loop);
    assert(model.get_next_loop() == &loop);
    assert(model.get_next_loop() == &loop);
  }
  {
    LOG(INFO) << "another loop";
    EventLoopThreadPool model(&loop, "another");
    model.set_thread_num(1);
    model.start(init);
    auto *next_loop = model.get_next_loop();
    next_loop->run_after(2, std::bind(print, next_loop));
    assert(next_loop != &loop);
    assert(model.get_next_loop() == next_loop);
    assert(model.get_next_loop() == next_loop);
    sleep(3);
  }
  {
    LOG(INFO) << "three threads";
    EventLoopThreadPool model(&loop, "three");
    model.set_thread_num(3);
    model.start(init);
    auto *next_loop = model.get_next_loop();
    next_loop->run_in_loop(std::bind(print, next_loop));
    assert(next_loop != &loop);
    assert(model.get_next_loop() != next_loop);
    assert(model.get_next_loop() != next_loop);
    assert(model.get_next_loop() == next_loop);
  }
  loop.loop();
}