#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "timestamp.h"

namespace muduo {

class Channel;
class Poller;
// class TimerId;
// class TimerQueue;

// 事件循环：one loop per thread 的实现
class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  // 阻止拷贝
  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;

  // 运行事件循环
  // 必须在EventLoop所属线程中调用
  void loop();
  void quit();

  // TimerId RunAt(Timestamp time, std::function<void()> cb);
  // TimerId RunAfter(double delay, std::function<void()> cb);
  // TimerId RunEvery(double interval, std::function<void()> cb);
  // void    Cancel(TimerId timerid);

  // if assert failed, abort the program.
  void assert_in_loop_thread() const;
  bool is_in_loop_thread() const;

  void run_in_loop(std::function<void()> func);
  void queue_in_loop(std::function<void()> func);

  void wakeup();
  void handle_read();

  static EventLoop *event_loop_of_current_thread();

  bool has_channel(Channel *);
  void remove_channel(Channel *);
  void update_channel(Channel *);

private:
  // 一个EventLoop有一个Poller，当前只实现了epoll
  std::unique_ptr<Poller> _poller;

  // 创建该EventLoop对象的线程ID，is_in_loop_thread用到
  const pid_t _threadid;
  int64_t     _iteration;

  // 是否处于事件循环当中
  std::atomic<bool> _looping;

  // 指示是否要退出事件循环
  std::atomic<bool> _quit;

  // 是否正在处理事件
  std::atomic<bool> _event_handling;

  // epoll_wait的返回的时间戳
  Timestamp _poll_return_ts;

  // epoll_wait返回的活动Channel
  std::vector<Channel *> _active_channels;

  // 当前正在处理的活动Channel
  Channel *_current_active_channel;

  int                                _wakeupfd;
  Channel                           *_wakeup_channel;
  std::mutex                         _mutex; // protects _pending_functors
  bool                               _calling_pending_functors;
  std::vector<std::function<void()>> _pending_functors;

  // // 处理定时器事件
  // std::unique_ptr<TimerQueue> _timer_queue;
};

} // namespace muduo
