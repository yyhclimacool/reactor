#include "muduo/src/event_loop.h"
#include "muduo/src/current_thread.h"

#include <glog/logging.h>
#include <memory>
#include <thread>

using namespace muduo;

void thread_func() {
    LOG(INFO) << "in thread_func: pid=" << getpid() << ", tid=" << tid();
    std::unique_ptr<EventLoop> loop(new EventLoop());
    loop->Loop();
}

int main() {
    LOG(INFO) << "in main: pid=" << getpid() << ", tid=" << tid();
    auto loop = std::make_unique<EventLoop>();
    std::thread bgt(thread_func);
    bgt.join();
    loop->Loop();

    return 0;
}
