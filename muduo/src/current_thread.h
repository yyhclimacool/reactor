#pragma once

namespace muduo {

extern __thread int  t_cached_tid;
extern __thread int  t_tid_string_length;
extern __thread char t_tid_string[ 32 ];

void cache_tid();

inline int tid() {
  if (__builtin_expect(t_cached_tid == 0, 0)) {
    cache_tid();
  }
  return t_cached_tid;
}

} // namespace muduo
