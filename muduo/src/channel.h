#pragma once

#include "timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

// A selectable I/O channel
//
// this class dosen't own the file descriptor
// a file descriptor could be a socket, eventfd, timerfd or a signalfd
class Channel {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    viod HandleEvent(Timestamp receive_time);

    void SetReadCallback(ReadEventCallback rcb) {
        _read_callback = std::move(rcb);
    }
    void SetWriteCallback(EventCallback wcb) {
        _write_callback = std::move(wcb);
    }
    void SetCloseCallback(EventCallback ccb) {
        _close_callback = std::move(ccb);
    }
    void SetErrorCallback(EventCallback ecb) {
        _error_callback = std::move(ecb);
    }

    // TODO: check this
    void tie(const std::shared_ptr<void> &);

    int Fd() const { return _fd; }
    int Events() const { return _events; }
    void SetRevents(int revt) { _revents = revt; }

    bool IsNoneEvent() const { return _events == kNoneEvent; }
    bool IsWriting() const { return _events & kWriteEvent; }
    bool IsReading() const { return _events & kReadEvent; }

    void EnableReading() { _events |= kReadEvent; Update(); }
    void DisableReading() { _events &= ~kReadEvent; Update(); }
    void EnableWriting() { _events |= kWriteEvent; Update(); }
    void DisableWriting() { _events &= ~kWriteEvent; Update(); }
    void DisableAll() { _events = kNoneEvent; Update(); }

    int Index() const { return _index; }
    void SetIndex(int idx) { _index = idx; }

    std::string ReventsToString() const;
    std::string EventsToString() const;

    void DisableLogHup() { _log_hup = false; }

    EventLoop *OwnerLoop() { return _loop; }
    void Remove();
private:
    static std::string EventsToString(int fd, int ev);

    void Update();
    void HandleEventWithGuard(Timestamp receive_time);
private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *_loop;
    const int _fd;
    int _events;
    int _revents; // received event types
    int _index; // used by poller.
    bool _log_hup;

    std::weak_ptr<void> _tie;
    bool _tied;
    bool _event_handling;
    bool _added_to_loop;

    ReadEventCallback _read_callback;
    EventCallback     _write_callback;
    EventCallback     _close_callback;
    EventCallback     _error_callback;
};
