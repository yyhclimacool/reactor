#include "current_thread.h"

__thread int t_cached_tid = 0;
__thread int t_tid_string_length = 6;
__thread char t_tid_string[32]{0};

void CacheTid() {
    if (t_cached_tid == 0) {
        t_cached_tid = static_cast<pid_t>(::syscall(SYS_gettid));
        t_tid_string_length = snprintf(t_tid_string, sizeof t_tid_string, "%5d ", t_cached_tid);
    }
}
