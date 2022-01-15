
#include <glog/logging.h>
#include <unistd.h>

#include "muduo/src/current_thread.h"
#include "muduo/src/event_loop.h"
#include "muduo/src/event_loop_thread.h"

using namespace muduo;

void print(EventLoop *loop = nullptr) {
  LOG(INFO) << "print: pid[" << getpid() << "], tid[" << tid() << "], loop[" << loop << "]";
}

void quit(EventLoop *loop) {
  LOG(INFO) << "quit: pid[" << getpid() << "], tid[" << tid() << "], loop[" << loop << "]";
  loop->quit();
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  print();
  {
    // 运行构造函数，运行loop函数，然后析构函数进行quit
    EventLoopThread loop_thread1;
  }
  LOG(INFO) << "================================================";
  {
    EventLoopThread loop_thread2(EventLoopThread::ThreadInitCallback(), "loop_thread2");
    auto           *loop = loop_thread2.start_loop();
    loop->run_in_loop(std::bind(print, loop));
    // 调用析构
  }
  LOG(INFO) << "================================================";
  {
    // quit before destructor
    EventLoopThread loop_thread3(EventLoopThread::ThreadInitCallback(), "loop_thread3");
    auto           *loop = loop_thread3.start_loop();
    loop->run_in_loop(std::bind(quit, loop));
    ::usleep(1000);
    // 调用析构
  }
}