#pragma once

#include <boost/noncopyable.hpp>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo {
class InetAddress;

// wrapper of socket file descriptor
// it closes the sockfd when destructs
// it's thread safe, all operations are delegated to OS.
class Socket : boost::noncopyable {
public:
  explicit Socket(int sockfd)
      : _sockfd(sockfd) {}
  ~Socket();

  int  fd() const { return _sockfd; }
  bool get_tcp_info(struct tcp_info *) const;
  bool get_tcp_info_string(char *buf, int len) const;
  // abort if address in use
  void bind_address(const InetAddress &localaddr);
  // abort if address in use
  void listen();

  // on success, returns a non-negative integer that is a descriptor for the accepted socket, which has been set to
  // non-blocking and close-on-exec. *peeraddr it assigned
  // on error, -1 is returned, and *peeraddr is untouched
  int accept(InetAddress *peeraddr);

  void shutdown_write();
  void set_tcp_nodelay(bool on);
  void set_reuse_addr(bool on);
  void set_reuse_port(bool on);
  void set_keep_alive(bool on);

private:
  const int _sockfd;
};
} // namespace muduo