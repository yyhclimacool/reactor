#include "channel.h"

Channel::Channel(EventLoop *loop, int fd)
    : _loop(loop),
      _fd(fd),
      _events(0),
      _revents(0),
      _index(-1),
      _log_hup(false),
      _tie(nullptr),
      _tied(false),
      _event_handling(false),
      _added_to_loop(false) {}

Channel::~Channel() {
    assert(!_event_handling);
    assert(!_added_to_loop);
    if (_loop->IsInLoopThread()) {
        assert(!_loop->HasChannel(this));
    }
}

void Channel::HandleEvent(Timestamp receive_time) {
    std::shared_ptr<void> guard;
    if (_tied) {
        guard = _tie.lock();
        if (guard) HandleEventWithGuard(receive_time);
    } else {
        HandleEventWithGuard(receive_time);
    }
}
