#include "inet_address.h"

#include <assert.h>
#include <glog/logging.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

#include "endian.h"
#include "sockops.h"

#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };
//
//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };
//
//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

using namespace muduo;

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6), "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family at offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family at offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port at offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port at offset 2");

// 不指定ip就用INADDR_ANY或者INADDR_LOOPBACK
InetAddress::InetAddress(uint16_t port, bool loop_back_only, bool ipv6) {
  static_assert(offsetof(InetAddress, _addr) == 0, "_addr at offset 0");
  static_assert(offsetof(InetAddress, _addr6) == 0, "_addr6 at offset 0");

  if (ipv6) {
    memset(&_addr6, 0x00, sizeof(_addr6));
    _addr6.sin6_family = AF_INET6;
    in6_addr ip = loop_back_only ? in6addr_loopback : in6addr_any;
    _addr6.sin6_addr = ip;
    _addr6.sin6_port = sockets::host_to_network16(port);
  } else {
    memset(&_addr, 0x00, sizeof(_addr));
    _addr.sin_family = AF_INET;
    in_addr_t ip = loop_back_only ? kInaddrLoopback : kInaddrAny;
    _addr.sin_addr.s_addr = sockets::host_to_network32(ip);
    _addr.sin_port = sockets::host_to_network16(port);
  }
}

// TODO: check string_view的用法
InetAddress::InetAddress(std::string_view ip, uint16_t port, bool ipv6) {
  if (ipv6 || strchr(ip.data(), ':')) {
    memset(&_addr6, 0x00, sizeof(_addr6));
    sockets::from_ip_port(ip.data(), port, &_addr6);
  } else {
    memset(&_addr, 0x00, sizeof(_addr));
    sockets::from_ip_port(ip.data(), port, &_addr);
  }
}

std::string InetAddress::to_ip_port() const {
  char buf[ 64 ] = "";
  sockets::to_ip_port(buf, sizeof(buf), get_sockaddr());
  return buf;
}

std::string InetAddress::to_ip() const {
  char buf[ 64 ] = "";
  sockets::to_ip(buf, sizeof(buf), get_sockaddr());
  return buf;
}

uint32_t InetAddress::ipv4_net_endian() const {
  assert(family() == AF_INET);
  return _addr.sin_addr.s_addr;
}

uint16_t InetAddress::port() const { return sockets::network_to_host16(port_net_endian()); }

static __thread char t_resolve_buf[ 64 * 1024 ];

bool InetAddress::resolve(std::string_view hostname, InetAddress* result) {
  assert(result != nullptr);
  struct hostent  hent;
  struct hostent* he = nullptr;
  int             herrno = 0;
  bzero(&hent, sizeof(hent));

  int ret = gethostbyname_r(hostname.data(), &hent, t_resolve_buf, sizeof(t_resolve_buf), &he, &herrno);
  if (ret == 0 && he != nullptr) {
    assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
    result->_addr.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
    return true;
  } else {
    if (ret) {
      LOG(ERROR) << "InetAddress::resolve error. hostname: " << hostname << " errno: " << errno;
    }
    return false;
  }
}

void InetAddress::set_scopeid(uint32_t scope_id) {
  if (family() == AF_INET6) {
    _addr6.sin6_scope_id = scope_id;
  }
}