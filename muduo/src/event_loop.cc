#include "current_thread.h"
#include "event_loop.h"
#include "poller.h"

#include <glog/logging.h>

thread_local EventLoop *t_LoopInThisThread = nullptr;

EventLoop::EventLoop() 
    : _poller(Poller::NewDefaultPoller(this)),
      _threadid(tid()),
      _iteration(0),
      _looping(false) {

    if(t_LoopInThisThread) {
        LOG(FATAL) << "another eventloop=" << t_LoopInThisThread << " exists in this thread=" << _threadid;
    } else {
        t_LoopInThisThread = this;
    }
}

EventLoop::~EventLoop() {
    LOG(INFO) << "eventloop=" << this << " of thread=" << _threadid << " destructs in thread=" << tid();
    t_LoopInThisThread = nullptr;
}

bool EventLoop::isInLoopThread() const {
    return _threadid == tid();
}

void EventLoop::loop() {
    assert(!_looping);
    assertInLoopThread();
}
