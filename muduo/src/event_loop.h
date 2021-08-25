#pragma once

#include "timestamp.h"

#include <atomic>
#include <memory>
#include <vector>


class Channel;
class Poller;

class EventLoop {
public:
    EventLoop();
    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;
    ~EventLoop();

    void Loop();

private:
    bool IsInLoopThread() const;
    void AssertInLoopThread() const;
    std::unique_ptr<Poller> _poller;
    const pid_t _threadid;
    int64_t _iteration;
    std::atomic<bool> _looping;
    std::atomic<bool> _quit;
    Timestamp _poolRetrunTime;
    std::vector<Channel *> _activeChannels;
    Channel * _currentActiveChannel;
};
