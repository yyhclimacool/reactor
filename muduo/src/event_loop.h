#pragma once

#include "timestamp.h"

#include <atomic>
#include <memory>
#include <vector>

class EventLoop {
public:
    EventLoop();
    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;
    ~EventLoop();

    void loop();

private:
    bool isInLoopThread() const;
    void assertInLoopThread() const;
    std::unique_ptr<Poller> _poller;
    const pid_t _threadid;
    int64_t _iteration;
    std::atomic<bool> _looping;
    std::atomic<bool> _quit;
    Timestamp _poolRetrunTime;
    std::vector<Channel *> _activeChannels;
    Channel * _currentActiveChannel;
};
