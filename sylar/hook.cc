#include "hook.h"
#include <dlfcn.h>
#include <unistd.h>
#include <functional>

#include "fiber.h"
#include "iomanager.h"

namespace sylar {
// 实现线程级别的hook
static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
  XX(sleep)          \
  XX(usleep)         \
  XX(nanosleep)      \
  XX(socket)         \
  XX(connect)        \
  XX(accept)         \
  XX(read)           \
  XX(readv)          \
  XX(recv)           \
  XX(recvfrom)       \
  XX(recvmsg)        \
  XX(write)          \
  XX(writev)         \
  XX(send)           \
  XX(sendto)         \
  XX(sendmsg)        \
  XX(close)          \
  XX(fcntl)          \
  XX(ioctl)          \
  XX(getsockopt)     \
  XX(setsockopt)

void hook_init() {
  static bool is_inited = false;
  if (is_inited) {
    return;
  }

#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
  HOOK_FUN(XX);
#undef XX
}

struct _HookIniter {
  _HookIniter() { hook_init(); }
};

// 在执行main函数之前，构造函数，在程序进来之前，Hook 函数和我们声明的变量关联起来  todo
static _HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }
void set_hook_enable(bool flag) { t_hook_enable = flag; }

}  // namespace sylar

extern "C" {
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
  if (!sylar::t_hook_enable) {
    return sleep_f(seconds);
  }

  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager *iom = sylar::IOManager::GetThis();
  //   iom->addEvent(seconds * 1000,
  // std::bind(&sylar::IOManager::schedule<sylar::Fiber::ptr>, iom, fiber));
  iom->addTimer(seconds * 1000, [iom, fiber]() { iom->schedule(fiber); });
  sylar::Fiber::YieldToHold();
  return 0;
}

int usleep(useconds_t usec) {
  if (!sylar::t_hook_enable) {
    return usleep_f(usec);
  }

  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager *iom = sylar::IOManager::GetThis();
  //   iom->addTimer(usec / 1000, std::bind(&sylar::IOManager::schedule, iom, fiber));
  iom->addTimer(usec / 1000, [iom, fiber]() { iom->schedule(fiber); });
  sylar::Fiber::YieldToHold();
  return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
  if (!sylar::t_hook_enable) {
    return nanosleep_f(req, rem);
  }

  int timout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager *iom = sylar::IOManager::GetThis();

  iom->addTimer(timout_ms, [iom, fiber]() { iom->schedule(fiber); });

  sylar::Fiber::YieldToHold();
  return 0;
}
}
