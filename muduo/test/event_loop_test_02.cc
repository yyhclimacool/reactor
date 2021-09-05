#include "muduo/src/event_loop.h"

#include <thread>

using namespace muduo;

EventLoop *g_loop;

void thread_func() {
    g_loop->Loop();
}

int main() {
    EventLoop loop;
    g_loop = &loop;
    std::thread bgt(thread_func);
    bgt.join();

    return 0;
}
