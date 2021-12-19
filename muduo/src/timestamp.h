#pragma once

#include <assert.h>

#include <boost/operators.hpp>
#include <string>

namespace muduo {

class Timestamp : public boost::equality_comparable<Timestamp>, public boost::less_than_comparable<Timestamp> {
  template <typename OSType>
  friend OSType &operator<<(OSType &, Timestamp t);

public:
  Timestamp() = default;
  explicit Timestamp(uint64_t ms_since_epoch)
      : _ms_since_epoch(ms_since_epoch) {}

  Timestamp &operator+=(double t_seconds) {
    _ms_since_epoch += static_cast<uint64_t>(t_seconds * kMicroSecondsPerSecond);
    return *this;
  }

  Timestamp operator+(double gap) { return operator+=(gap); }

  // compatable with STL
  void swap(Timestamp &other) {
    using std::swap;
    swap(_ms_since_epoch, other._ms_since_epoch);
  }

  std::string to_string() const;
  std::string to_formatted_string(bool show_ms = true) const;
  bool        valid() const { return _ms_since_epoch > 0; }
  uint64_t    ms_since_epoch() const { return _ms_since_epoch; }
  time_t      seconds_since_epoch() const { return _ms_since_epoch / kMicroSecondsPerSecond; }

  static Timestamp now();
  static Timestamp invalid() { return Timestamp(); }
  static Timestamp from_unix_time(time_t t) { return from_unix_time(t, 0); }
  static Timestamp from_unix_time(time_t t, uint64_t ms) {
    return Timestamp(static_cast<uint64_t>(t) * kMicroSecondsPerSecond + ms);
  }

public:
  static const uint64_t kMicroSecondsPerSecond = 1000 * 1000;
  uint64_t              _ms_since_epoch = 0;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) { return lhs._ms_since_epoch < rhs._ms_since_epoch; }

inline bool operator==(Timestamp lhs, Timestamp rhs) { return lhs._ms_since_epoch == rhs._ms_since_epoch; }

inline int64_t operator-(Timestamp lhs, Timestamp rhs) {
  assert(rhs < lhs);
  return static_cast<int64_t>(lhs._ms_since_epoch - rhs._ms_since_epoch);
}

template <typename OSType>
OSType &operator<<(OSType &os, Timestamp t) {
  return os << t.to_formatted_string();
}

} // namespace muduo
