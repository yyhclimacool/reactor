#include "muduo/src/epoll_poller.h"
#include "muduo/src/channel.h"

#include <glog/logging.h>
#include <sys/epoll.h>
#include <sys/poll.h>

static_assert(EPOLLIN == POLLIN,       "EPOLLIN equals to POLLIN");
static_assert(EPOLLPRI == POLLPRI,     "EPOLLPRI equals to POLLPRI");
static_assert(EPOLLOUT == POLLOUT,     "EPOLLOUT equals to POLLOUT");
static_assert(EPOLLRDHUP == POLLRDHUP, "EPOLLRDHUP equals to POLLRDHUP");
static_assert(EPOLLERR == POLLERR,     "EPOLLERR equals to POLLERR");
static_assert(EPOLLHUP == POLLHUP,     "EPOLLHUP equals to POLLHUP");

namespace {
    const int kNew = -1;
    const int kAdded = 1;
    const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop *loop) 
    : Poller(loop),
      _epollfd(epoll_create1(EPOLL_CLOEXEC)),
      _events(kInitEventListSize) {
    if (_epollfd < 0) {
        LOG(FATAL) << "create epoll fd failed, loop=" << loop;
    }
}

EPollPoller::~EPollPoller() {
    close(_epollfd);
}

Timestamp EPollPoller::Poll(int timeout_ms, std::vector<Channel *> *active_channels) {
    LOG(INFO) << "total fd num=" << _channels.size() << " in epoll[" << this << "]";
    int num_events = epoll_wait(_epollfd,
                                _events.data(),
                                static_cast<int>(_events.size()),
                                timeout_ms);
    int saved_errno = errno;
    auto now = Timestamp::Now();
    if (num_events > 0) {
        LOG(INFO) << "num_events=" << num_events << " happend in epoll[" << this << "]";
        FillActiveChannels(num_events, active_channels);
        if (static_cast<size_t>(num_events) == _events.size()) {
            _events.resize(_events.size() * 2);
        }
    } else if (num_events == 0) {
        LOG(INFO) << "nothing happend in epoll[" << this << "]";
    } else {
        // error happened
        if (saved_errno != EINTR) {
            errno = saved_errno;
            LOG(WARNING) << "caused an error_no=" << saved_errno << " in epoll[" << this <<"]";
        }
    }

    return now;
}

void EPollPoller::FillActiveChannels(int num_events, std::vector<Channel *> *active_channels) const {
    assert(static_cast<size_t>(num_events) <= _events.size());
    for (int i = 0; i < num_events; ++i) {
        Channel *ch = static_cast<Channel *>(_events[i].data.ptr);
#ifndef NDEBUG
        int fd = ch->Fd();
        auto it = _channels.find(fd);
        assert(it != _channels.end());
        assert(it->second == ch);
#endif
        ch->SetRevents(_events[i].events);
        active_channels->push_back(ch);
    }
}

void EPollPoller::UpdateChannel(Channel *ch) {
    AssertInLoopThread();
    const int index = ch->Index();
    LOG(INFO) << "fd=" << ch->Fd() << ", events=" << ch->Events() << ", index=" << ch->Index();
    if (index == kNew || index == kDeleted) {
        int fd = ch->Fd();
        // if it's a new channel.
        if (index == kNew) {
            assert(_channels.find(fd) == _channels.end());
            _channels[fd] = ch;
        } else { // if this channel is marked deleted, but not real deleted from map.
            assert(_channels.find(fd) != _channels.end());
            assert(_channels[fd] == ch);
        }
        Update(EPOLL_CTL_ADD, ch);
        // TODO : check
        ch->SetIndex(kAdded);
    } else {
        int fd = ch->Fd();
        assert(_channels.find(fd) != _channels.end());
        assert(_channels[fd] == ch);
        assert(index == kAdded);
        if (ch->IsNoneEvent()) {
            // marked as non-event, delete it and mark it's index as kDeleted.
            Update(EPOLL_CTL_DEL, ch);
            ch->SetIndex(kDeleted);
        } else {
            // not non-event, but actually it's event has changed, so update to epoll.
            Update(EPOLL_CTL_MOD, ch);
        }
    }
}

// 这里的channel的index表示的channel的状态
// 不论channel是哪种状态，调用该函数都是将该channel从epoll poller中彻底删除，包括从epollfd中删除，也从channel map中删除
// 如果channel是kAdded状态，处理逻辑稍微不同
// 最终都将channel置为kNew的状态
void EPollPoller::RemoveChannel(Channel *ch) {
    AssertInLoopThread();
    int fd = ch->Fd();
    assert(_channels.find(fd) != _channels.end());
    assert(_channels[fd] == ch);
    assert(ch->IsNoneEvent());
    int index = ch->Index();
    assert(index == kAdded || index == kDeleted);
    size_t n = _channels.erase(fd);
    (void)n;
    assert(n == 1);
    if (index == kAdded) {
        Update(EPOLL_CTL_DEL, ch);
    }
    ch->SetIndex(kNew);
}

const char *EPollPoller::OperationToString(int op) {
    switch(op) {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            assert(false && "ERROR OP");
            return "Invalid Operation";
    }
}

void EPollPoller::Update(int op, Channel *ch) {
    struct epoll_event event;
    memset(&event, 0x00, sizeof(event));
    event.events = ch->Events();
    event.data.ptr = ch;
    int fd = ch->Fd();
    LOG(INFO) << "epoll_ctl op=" << OperationToString(op) << ", fd=" << fd << ", event=" << ch->EventsToString();
    if (epoll_ctl(_epollfd, op, fd, &event) < 0) {
        if (op == EPOLL_CTL_DEL) {
            LOG(WARNING) << "epoll_ctl_del for fd=" << fd << " failed.";
        } else {
            LOG(WARNING) << "epoll_ctl op=" << OperationToString(op) << " for fd=" << fd << " failed.";
        }
    }
}

Poller *Poller::NewDefaultPoller(EventLoop *loop) {
    return new EPollPoller(loop);
}
