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

void Channel::Tie(const std::shared_ptr<void> &obj) {
    _tie = obj;
    _tied = true;
}

std::string Channel::ReventsToString() const {
    return EventsToString(_fd, _revents);
}

std::string Channel::EventsToString() const {
    return EventsToString(_fd, _events);
}

std::string Channel::EventsToString(int fd, int ev) {
    std::ostringstream oss;
    oss << "fd=" << _fd << ":";
    if (ev & POLLIN)    oss << "IN ";
    if (ev & POLLPRI)   oss << "PRI ";
    if (ev & POLLOUT)   oss << "OUT ";
    if (ev & POLLHUP)   oss << "HUP ";
    if (ev & POLLRDHUP) oss << "RDHUP ";
    if (ev & POLLERR)   oss << "ERR ";
    if (ev & POLLNVAL)  oss << "NVAL ";

    return oss.str();
}

void Channnel::Remove() {
    assert(IsNoneEvent());
    _added_to_loop = false;
    _loop->RemoveChannel(this);
}

void Channnel::Update() {
    _added_to_loop = true;
    _loop->UpdateChannel(this);
}
