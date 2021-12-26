#include "acceptor.h"

#include <errno.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <unistd.h>

#include "event_loop.h"
#include "inet_address.h"
#include "sockops.h"

using namespace muduo;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listen_addr, bool reuse_port)
    : _loop(loop)
    , _accept_socket(sockets::create_nonblocking_or_die(listen_addr.family()))
    , _accept_channel(loop, _accept_socket.fd())
    , _listening(false)
    , _idlefd(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  assert(_idlefd >= 0);
  _accept_socket.set_reuse_addr(true);
  _accept_socket.set_reuse_port(reuse_port);
  _accept_socket.bind_address(listen_addr);
  _accept_channel.set_read_callback(std::bind(&Acceptor::handle_read, this));
}

Acceptor::~Acceptor() {
  _accept_channel.disable_all();
  _accept_channel.remove();
  sockets::close(_idlefd);
}

// TODO：为什么listen要在I/O线程中调用呢？
void Acceptor::listen() {
  _loop->assert_in_loop_thread();
  _listening = true;
  _accept_socket.listen();
  _accept_channel.enable_reading();
}

// channel的handle_read肯定是在I/O线程中调用的
void Acceptor::handle_read() {
  _loop->assert_in_loop_thread();
  InetAddress peeraddr;
  // FIXME: loop until no more
  int connfd = _accept_socket.accept(&peeraddr);
  if (connfd >= 0) {
    if (_new_connection_callback) {
      _new_connection_callback(connfd, peeraddr);
    } else {
      sockets::close(connfd);
    }
  } else {
    LOG(ERROR) << "Acceptor::handle_read error.";
    if (errno == EMFILE) {
      ::close(_idlefd);
      _idlefd = ::accept(_accept_socket.fd(), NULL, NULL);
      ::close(_idlefd);
      _idlefd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}