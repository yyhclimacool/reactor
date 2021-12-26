#include "socket.h"

#include <glog/logging.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h> // snprintf

#include "inet_address.h"
#include "sockops.h"

using namespace muduo;

Socket::~Socket() { sockets::close(_sockfd); }

bool Socket::get_tcp_info(struct tcp_info *tcpi) const {
  socklen_t len = sizeof(*tcpi);
  memset(tcpi, 0x00, len);
  return ::getsockopt(_sockfd, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::get_tcp_info_string(char *buf, int len) const {
  struct tcp_info tcpi;
  bool            ok = get_tcp_info(&tcpi);
  if (ok) {
    snprintf(buf, len,
             "unrecovered[%u] "
             "rto[%u] ato[%u] snd_mss[%u] rcv_mss[%u] "
             "lost[%u] retrans[%u] rtt[%u] rttvar[%u] "
             "sshthresh[%u] cwnd[%u] total_retrans[%u]",
             tcpi.tcpi_retransmits, // Number of unrecovered [RTO] timeouts
             tcpi.tcpi_rto,         // Retransmit timeout in usec
             tcpi.tcpi_ato,         // Predicted tick of soft clock in usec
             tcpi.tcpi_snd_mss, tcpi.tcpi_rcv_mss,
             tcpi.tcpi_lost,    // Lost packets
             tcpi.tcpi_retrans, // Retransmitted packets out
             tcpi.tcpi_rtt,     // Smoothed round trip time in usec
             tcpi.tcpi_rttvar,  // Medium deviation
             tcpi.tcpi_snd_ssthresh, tcpi.tcpi_snd_cwnd,
             tcpi.tcpi_total_retrans); // Total retransmits for entire connection
  }
  return ok;
}

void Socket::bind_address(const InetAddress &addr) { sockets::bind_or_die(_sockfd, addr.get_sockaddr()); }

void Socket::listen() { sockets::listen_or_die(_sockfd); }

int Socket::accept(InetAddress *peeraddr) {
  struct sockaddr_in6 addr;
  memset(&addr, 0x00, sizeof addr);
  int connfd = sockets::accept(_sockfd, &addr);
  if (connfd >= 0) {
    peeraddr->set_sockaddr_in6(addr);
  }
  return connfd;
}

void Socket::shutdown_write() { sockets::shutdown_write(_sockfd); }

void Socket::set_tcp_nodelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::set_reuse_addr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::set_reuse_port(bool on) {
#ifdef SO_REUSEPORT
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
  if (ret < 0 && on) {
    LOG(ERROR) << "SO_REUSEPORT failed.";
  }
#else
  if (on) {
    LOG(ERROR) << "SO_REUSEPORT is not supported.";
  }
#endif
}

void Socket::set_keep_alive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}