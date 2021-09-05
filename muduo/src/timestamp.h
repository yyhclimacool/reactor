#pragma once

#include <boost/operators.hpp>

#include <assert.h>
#include <string>

namespace muduo {

class Timestamp : public boost::equality_comparable<Timestamp>,
                  public boost::less_than_comparable<Timestamp> {
public:
    template<typename OS>
    friend OS &operator<<(OS &, Timestamp t);
    Timestamp & operator+=(double t) {
        _ms_since_epoch += static_cast<uint64_t>(t * kMicroSecondsPerSecond);
        return *this;
    }
    
    Timestamp() = default;
    explicit Timestamp(uint64_t ms_since_epoch) : _ms_since_epoch(ms_since_epoch) {}

    // Compatable with STL
    void swap(Timestamp &other) {
        using std::swap;
        swap(_ms_since_epoch, other._ms_since_epoch);
    }

    std::string ToString() const;
    std::string ToFormattedString(bool show_ms = true) const;
    bool Valid() const { return _ms_since_epoch > 0; }
    uint64_t MicroSecondsSinceEpoch() const { return _ms_since_epoch; }
    time_t SecondsSinceEpoch() const { return _ms_since_epoch/kMicroSecondsPerSecond; }

    static Timestamp Now();
    static Timestamp Invalid() { return Timestamp(); }
    static Timestamp FromUnixTime(time_t t) { return FromUnixTime(t, 0); }
    static Timestamp FromUnixTime(time_t t, uint64_t ms) {
        return Timestamp(static_cast<uint64_t>(t) * kMicroSecondsPerSecond + ms);
    }
public:
    static const uint64_t kMicroSecondsPerSecond = 1000 * 1000;
    uint64_t _ms_since_epoch = 0;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs._ms_since_epoch < rhs._ms_since_epoch;
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs._ms_since_epoch == rhs._ms_since_epoch;
}

inline double operator-(Timestamp lhs, Timestamp rhs) {
    assert(rhs < lhs);
    return static_cast<double>(lhs._ms_since_epoch - rhs._ms_since_epoch)/Timestamp::kMicroSecondsPerSecond;
}

template<typename OS>
OS &operator<<(OS &os, Timestamp t) {
    os << t.ToFormattedString();
}

} // namespace muduo
