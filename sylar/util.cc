#include "util.h"
#include <cstdint>

namespace sylar {
pid_t GetThreadId() { return syscall(SYS_gettid); }
uint32_t GetFiberId() { return 0; }
}  // namespace sylar