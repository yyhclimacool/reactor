#include "current_thread.h"

#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

using namespace muduo;

__thread int  muduo::t_cached_tid = 0;
__thread int  muduo::t_tid_string_length = 6;
__thread char muduo::t_tid_string[ 32 ]{0};

void muduo::cache_tid() {
  if (muduo::t_cached_tid == 0) {
    muduo::t_cached_tid = static_cast<pid_t>(::syscall(SYS_gettid));
    t_tid_string_length = snprintf(t_tid_string, sizeof t_tid_string, "%5d ", muduo::t_cached_tid);
  }
}
