#pragma once

#include "timestamp.h"

#include <atomic>
#include <memory>
#include <vector>

class Channel;
class Poller;

// 事件循环：one loop per thread 的实现
//
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    // 阻止拷贝
    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;

    // 运行事件循环
    // 必须在EventLoop所属线程中调用
    void Loop();
    void Quit();

    // if assert failed, abort the program.
    void AssertInLoopThread() const;
    bool IsInLoopThread() const;

    static EventLoop *EventLoopOfCurrentThread();

    bool HasChannel(Channel *);
    void RemoveChannel(Channel *);
    void UpdateChannel(Channel *);

private:
    // 一个EventLoop有一个Poller，当前只实现了epoll
    std::unique_ptr<Poller> _poller;

    // 创建该EventLoop对象的线程ID
    const pid_t _threadid;
    int64_t _iteration;

    // 是否处于事件循环当中
    std::atomic<bool> _looping;

    // 指示是否要退出事件循环
    std::atomic<bool> _quit;

    // 是否正在处理事件
    std::atomic<bool> _event_handling;

    // epoll_wait的返回的时间戳
    Timestamp _poll_return_time;

    // epoll_wait返回的活动Channel
    std::vector<Channel *> _active_channels;

    // 当前正在处理的活动Channel
    Channel * _current_active_channel;
};
