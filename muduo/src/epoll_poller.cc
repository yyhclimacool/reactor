#include "epoll_poller.h"
#include <glog/logging.h>

std::static_assert(EPOLLIN == POLLIN, "EPOLLIN equals to POLLIN");
std::static_assert(EPOLLPRI == POLLPRI, "EPOLLPRI equals to POLLPRI");
std::static_assert(EPOLLOUT == POLLOUT, "EPOLLOUT equals to POLLOUT");
std::static_assert(EPOLLRDHUP == POLLRDHUP, "EPOLLRDHUP equals to POLLRDHUP");
std::static_assert(EPOLLERR == POLLERR, "EPOLLERR equals to POLLERR");
std::static_assert(EPOLLHUP == POLLHUP, "EPOLLHUP equals to POLLHUP");

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

void EPollPoller::FillActiveChannels(int num_events, std::vector<Channel *> *active_channels) {
    assert(static_cast<size_t>(num_events) <= _events.size());
    for (int i = 0; i < num_events; ++i) {
        Channel *ch = static_cast<Channel *>(_events[i].data.ptr);
#ifndef NDEBUG
        int fd = ch->fd();
        auto it = _channels.find(fd);
        assert(it != _channels.end());
        assert(it->second == ch);
#endif
        ch->SetREvents(_events[i].events);
        active_channels->push_back(ch);
    }
}

void EPollPoller::UpdataChannel(Channel *ch) {
    AssertInLoopThread();
    const int index = ch->index();
    LOG(INFO) << "fd=" << ch->fd() << ", events=" << ch->events() << ", index=" << ch->index();
    if (index == kNew || index == kDeleted) {
        int fd = ch->fd();
        if (index == kNew) {
            assert(_channels.find(fd) == _channels.end());
            _channels[fd] = ch;
        } else {
            assert(_channels.find(fd) != _channels.end());
            assert(_channels[fd] == ch);
        }
        ch->set_index(kAdded);
        Update(EPOLL_CTL_ADD, ch);
    } else {
        int fd = ch->fd();
        (void) fd;
        assert(_channels.find(fd) != _channels.end());
        assert(_channels[fd] == ch);
        assert(index == kAdded);
        if (ch->isNoneEvent()) {
            ch->set_index(kDeleted);
            Update(EPOLL_CTL_DEL, ch);
        } else {
            Update(EPOLL_CTL_MOD, ch);
        }
    }
}
