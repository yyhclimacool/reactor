#include "sockops.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <stdio.h> // snprintf
#include <sys/socket.h>
#include <sys/uio.h> // readv
#include <unistd.h>

#include "endian.h"

using namespace muduo;

namespace {
using SA = struct sockaddr;
} // namespace

const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in6 *addr) {
  return static_cast<const SA *>(static_cast<const void *>(addr));
}
struct sockaddr *sockets::sockaddr_cast(struct sockaddr_in6 *addr) {
  return static_cast<SA *>(static_cast<void *>(addr));
}
const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in *addr) {
  return static_cast<const SA *>(static_cast<const void *>(addr));
}
const struct sockaddr_in *sockets::sockaddr_in_cast(const struct sockaddr *addr) {
  return static_cast<const struct sockaddr_in *>(static_cast<const void *>(addr));
}
const struct sockaddr_in6 *sockets::sockaddr_in6_cast(const struct sockaddr *addr) {
  return static_cast<const struct sockaddr_in6 *>(static_cast<const void *>(addr));
}

int sockets::create_nonblocking_or_die(sa_family_t family) {
  int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG(FATAL) << "sockets::create_nonblocking_or_die failed.";
  }
  return sockfd;
}

void sockets::bind_or_die(int sockfd, const struct sockaddr *addr) {
  int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    LOG(FATAL) << "sockets::bind_or_die failed.";
  }
}

void sockets::listen_or_die(int sockfd) {
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0) {
    LOG(FATAL) << "sockets::listen_or_die failed.";
  }
}

int sockets::accept(int sockfd, struct sockaddr_in6 *addr) {
  socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
  int       connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd < 0) {
    int saved_errno = errno;
    LOG(ERROR) << "Socket::accept error " << saved_errno;
    switch (saved_errno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO:
      case EPERM:
      case EMFILE: // per-process limit of open file desctiptor
        // expected errors
        errno = saved_errno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        LOG(FATAL) << "unexpected error of ::accept4 " << saved_errno;
        break;
      default:
        LOG(FATAL) << "unknown error of ::accept4 " << saved_errno;
        break;
    }
  }
  return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr *addr) {
  return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t sockets::read(int sockfd, void *buf, size_t count) { return ::read(sockfd, buf, count); }

ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt) { return ::readv(sockfd, iov, iovcnt); }

ssize_t sockets::write(int sockfd, const void *buf, size_t count) { return ::write(sockfd, buf, count); }

void sockets::close(int sockfd) {
  if (::close(sockfd) < 0) {
    LOG(ERROR) << "sockets::close error.";
  }
}

void sockets::shutdown_write(int sockfd) {
  if (::shutdown(sockfd, SHUT_WR) < 0) {
    LOG(ERROR) << "sockets::shutdown_write error.";
  }
}

void sockets::to_ip_port(char *buf, size_t size, const struct sockaddr *addr) {
  if (addr->sa_family == AF_INET6) {
    // NOTE: ipv6需要加方括号
    buf[ 0 ] = '[';
    to_ip(buf + 1, size - 1, addr);
    size_t                     end = ::strlen(buf);
    const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
    uint16_t                   port = sockets::network_to_host16(addr6->sin6_port);
    assert(size > end);
    snprintf(buf + end, size - end, "]:%u", port);
    return;
  }
  to_ip(buf, size, addr);
  size_t                    end = ::strlen(buf);
  const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
  uint16_t                  port = sockets::network_to_host16(addr4->sin_port);
  assert(size > end);
  snprintf(buf + end, size - end, ":%u", port);
}

void sockets::to_ip(char *buf, size_t size, const struct sockaddr *addr) {
  if (addr->sa_family == AF_INET) {
    assert(size >= INET_ADDRSTRLEN);
    const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
    ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
  } else if (addr->sa_family == AF_INET6) {
    assert(size >= INET6_ADDRSTRLEN);
    const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
    ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
  }
}

void sockets::from_ip_port(const char *ip, uint16_t port, struct sockaddr_in *addr) {
  addr->sin_family = AF_INET;
  addr->sin_port = host_to_network16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
    LOG(ERROR) << "sockets::from_ip_port error";
  }
}

void sockets::from_ip_port(const char *ip, uint16_t port, struct sockaddr_in6 *addr) {
  addr->sin6_family = AF_INET6;
  addr->sin6_port = host_to_network16(port);
  if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
    LOG(ERROR) << "sockets::from_ip_port error";
  }
}

int sockets::get_socket_error(int sockfd) {
  int       optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  } else {
    return optval;
  }
}

struct sockaddr_in6 sockets::get_local_addr(int sockfd) {
  struct sockaddr_in6 localaddr;
  memset(&localaddr, 0, sizeof localaddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
  if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
    LOG(ERROR) << "sockets::get_local_addr error";
  }
  return localaddr;
}

struct sockaddr_in6 sockets::get_peer_addr(int sockfd) {
  struct sockaddr_in6 peeraddr;
  memset(&peeraddr, 0, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
    LOG(ERROR) << "sockets::get_peer_addr error";
  }
  return peeraddr;
}

bool sockets::is_self_connect(int sockfd) {
  struct sockaddr_in6 localaddr = get_local_addr(sockfd);
  struct sockaddr_in6 peeraddr = get_peer_addr(sockfd);
  if (localaddr.sin6_family == AF_INET) {
    const struct sockaddr_in *laddr4 = reinterpret_cast<struct sockaddr_in *>(&localaddr);
    const struct sockaddr_in *raddr4 = reinterpret_cast<struct sockaddr_in *>(&peeraddr);
    return laddr4->sin_port == raddr4->sin_port && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
  } else if (localaddr.sin6_family == AF_INET6) {
    return localaddr.sin6_port == peeraddr.sin6_port &&
           memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
  } else {
    return false;
  }
}