#pragma once

#include <netinet/in.h>

#include <boost/noncopyable.hpp>
#include <string_view>

namespace muduo {
namespace sockets {
const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
}

// wrapper of sockaddr_in
// this is an POD interface class
class InetAddress : public boost::noncopyable {
public:
  explicit InetAddress(uint16_t port = 0, bool loop_back_only = false, bool ipv6 = false);
  // @c ip should be "1.2.3.4"
  InetAddress(std::string_view ip, uint16_t port, bool ipv6 = false);
  explicit InetAddress(const struct sockaddr_in &addr)
      : _addr(addr) {}
  explicit InetAddress(const struct sockaddr_in6 &addr)
      : _addr6(addr) {}
  sa_family_t family() const { return _addr.sin_family; }
  std::string to_ip() const;
  std::string to_ip_port() const;
  uint16_t    port() const;

  // TODO: 为什么要从addr6 cast过来呢？
  const struct sockaddr *get_sockaddr() const { return sockets::sockaddr_cast(&_addr6); }
  void                   set_sockaddr_in6(const struct sockaddr_in6 &addr6) { _addr6 = addr6; }
  uint32_t               ipv4_net_endian() const;
  uint16_t               port_net_endian() const { return _addr.sin_port; }

  // resolve hostname to ip address, not changing port or sin_family
  // return true on success
  // thread safe
  static bool resolve(std::string_view hostname, InetAddress *result);

  // set IPv6 scope id
  void set_scopeid(uint32_t scopeid);

private:
  union {
    struct sockaddr_in  _addr;
    struct sockaddr_in6 _addr6;
  };
};

} // namespace muduo