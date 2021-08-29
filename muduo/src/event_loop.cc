#include "channel.h"
#include "current_thread.h"
#include "event_loop.h"
#include "poller.h"

#include <glog/logging.h>

thread_local EventLoop *t_LoopInThisThread = nullptr;

EventLoop::EventLoop() 
    : _poller(Poller::NewDefaultPoller(this)),
      _threadid(tid()),
      _iteration(0),
      _looping(false),
      _quit(false) {
    // 如果当前线程的EventLoop已经存在，严重错误，退出程序
    if(t_LoopInThisThread) {
        LOG(FATAL) << "another eventloop=" << t_LoopInThisThread << " exists in this thread=" << tid();
    } else {
        t_LoopInThisThread = this;
    }
    LOG(INFO) << "create an event_loop=" << this << " in thread=" << _threadid;
}

EventLoop::~EventLoop() {
    LOG(INFO) << "eventloop=" << this << " of thread=" << _threadid << " destructs in thread=" << tid();
    // 置空
    t_LoopInThisThread = nullptr;
}

EventLoop *EventLoop::EventLoopOfCurrentThread() {
    return t_LoopInThisThread;
}

bool EventLoop::IsInLoopThread() const {
    return _threadid == tid();
}

void EventLoop::AssertInLoopThread() const {
    if (!IsInLoopThread()) {
        LOG(FATAL) << "AssertInLoopThread failed. abort program. event_loop=" << this << ", created thread_id=" << _threadid << ", current threadid=" << tid();
    }
}

// 只能在创建对象的线程中调用
// 该线程称作I/O线程
void EventLoop::Loop() {
    assert(!_looping);
    AssertInLoopThread();
    _looping = true;
    LOG(INFO) << "event_loop=" << this << " start looping ...";
    std::vector<Channel *> active_channels;
    auto ts = _poller->Poll(100/*ms*/, &active_channels);
    LOG(INFO) << "event_loop=" << this << " stop looping.";
    _looping = false;
}

bool EventLoop::HasChannel(Channel *ch) {
    assert(ch->OwnerLoop() == this);
    AssertInLoopThread();
    return _poller->HasChannel(ch);
}

void EventLoop::RemoveChannel(Channel *ch) {
    assert(ch->OwnerLoop() == this);
    AssertInLoopThread();
    if (_event_handling) {
        assert(_current_active_channel == ch ||
                std::find(_active_channels.begin(), _active_channels.end(), ch) == _active_channels.end());
    }
    _poller->RemoveChannel(ch);
}

void EventLoop::UpdateChannel(Channel *ch) {
    assert(ch->OwnerLoop() == this);
    AssertInLoopThread();
    _poller->UpdateChannel(ch);
}
