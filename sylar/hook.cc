#include "hook.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <cstdarg>
#include <memory>

#include "config.h"
#include "fd_manager.h"
#include "fiber.h"
#include "iomanager.h"
#include "log.h"
#include "macro.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace sylar {

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

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

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
  _HookIniter() {
    hook_init();
    s_connect_timeout = g_tcp_connect_timeout->getValue();

    g_tcp_connect_timeout->addListener(
        [](const int &old_value, const int &new_value) {
          SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                   << old_value << "to" << new_value;
          s_connect_timeout = new_value;
        });
  }
};

// 这里确保 在main函数之前，就初始化好
static _HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }

void set_hook_enable(bool flag) { t_hook_enable = flag; }

}  // namespace sylar

struct timer_info {
  int cancelled = 0;
};

// 通用模板函数
template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args) {
  if (!sylar::t_hook_enable) {
    //  read(int fd, void *buf, size_t count)
    //  return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO,
    //  buf, count);
    return fun(fd, std::forward<Args>(args)...);
  }

  SYLAR_LOG_DEBUG(g_logger) << "do_io <" << hook_fun_name << ">";
  // 得到fd 对应的 FdCtx
  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
  if (!ctx) {
    return fun(fd, std::forward<Args>(args)...);
  }
  if (ctx->isClosed()) {
    // Bad file descriptor
    errno = EBADF;
    return -1;
  }

  // 判断传入的fd是否为套接字，如果不为套接字，则调用系统的connect函数并返回。
  // 判断fd是否被显式设置为了非阻塞模式，如果是则调用系统的connect函数并返回。
  if (!ctx->isSocket() || ctx->getUserNonblock()) {
    return fun(fd, std::forward<Args>(args)...);
  }

  uint64_t to = ctx->getTimeout(timeout_so);
  // todo  make_shared
  std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();

retry:
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  // EINTR ---Interrupted system call
  while (n == -1 && errno == EINTR) {
    n = fun(fd, std::forward<Args>(args)...);
  }
  if (n == -1 && errno == EAGAIN) {
    SYLAR_LOG_DEBUG(g_logger) << "do_io" << hook_fun_name << ">";
    sylar::IOManager *iom = sylar::IOManager::GetThis();
    sylar::Timer::ptr timer;
    std::weak_ptr<timer_info> winfo(tinfo);

    // (uint64_t)-1 = 18446744073709551615 (0xffffffffffffffff)
    if (to != (uint64_t)-1) {
      timer = iom->addConditionTimer(
          to,
          [winfo, fd, iom, event]() {
            auto t = winfo.lock();
            if (!t || t->cancelled) {
              return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, (sylar::IOManager::Event)(event));
          },
          winfo);
    }

    /// 若读阻塞说明 缓冲区空 写阻塞 缓冲区满
    /// 添加 可读可写事件，即可获取协程唤醒的标志
    int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));
    if (rt) {
      SYLAR_LOG_ERROR(g_logger)
          << hook_fun_name << " addEvent(" << fd << ", " << event << ")";

      if (timer) {
        timer->cancel();
      }
      return -1;

    } else {
      // SYLAR_LOG_ERROR(g_logger) << "do_io<" << hook_fun_name << ">";
      sylar::Fiber::YieldToHold();
      // SYLAR_LOG_ERROR(g_logger) << "do_io<" << hook_fun_name << ">";
      if (timer) {
        timer->cancel();
      }
      if (tinfo->cancelled) {
        errno = tinfo->cancelled;
        return -1;
      }

      goto retry;  // todo
    }
  }

  return n;
}

#ifdef __cplusplus

extern "C" {

#endif

#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
  if (!sylar::t_hook_enable) {
    return sleep_f(seconds);
  }
  // 返回当前线程正在执行的协程
  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  // 用于获取当前线程的调度器的调度协程
  sylar::IOManager *iom = sylar::IOManager::GetThis();
  iom->addTimer(
      seconds * 1000,
      std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) &
                    sylar::IOManager::schedule,
                iom, fiber, -1));

  sylar::Fiber::YieldToHold();
  return 0;
}

int usleep(useconds_t usec) {
  if (!sylar::t_hook_enable) {
    return usleep_f(usec);
  }

  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager *iom = sylar::IOManager::GetThis();
  iom->addTimer(
      usec / 1000,
      std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) &
                    sylar::IOManager::schedule,
                iom, fiber, -1));

  sylar::Fiber::YieldToHold();
  return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
  if (!sylar::t_hook_enable) {
    return nanosleep_f(req, rem);
  }

  int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager *iom = sylar::IOManager::GetThis();

  iom->addTimer(
      timeout_ms,
      std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) &
                    sylar::IOManager::schedule,
                iom, fiber, -1));

  sylar::Fiber::YieldToHold();
  return 0;
}

// socket
int socket(int domain, int type, int protocol) {
  if (!sylar::t_hook_enable) {
    return socket_f(domain, type, protocol);
  }
  int fd = socket_f(domain, type, protocol);
  if (fd == -1) {
    return fd;
  }
  // true 表示,FdMgr没有时会自动创建
  sylar::FdMgr::GetInstance()->get(fd, true);
  return fd;
}
// todo
// int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t
// addrlen,
//                          uint64_t timeout_ms) {
//   if (!sylar::t_hook_enable) {
//     return connect_f(fd, addr, addrlen);
//   }
//   sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
//   if (!ctx || ctx->isClosed()) {
//     errno = EBADF;
//     return -1;
//   }

//   if (!ctx->isSocket()) {
//     return connect_f(fd, addr, addrlen);
//   }
//   if (ctx->getUserNonblock()) {
//     return connect_f(fd, addr, addrlen);
//   }

//   size_t n = connect_f(fd, addr, addrlen);
//   if (n == 0) {
//     return 0;
//   } else if (n != (uint64_t)-1) {
//     return n;
//   }

//   sylar::IOManager *iom = sylar::IOManager::GetThis();
//   sylar::Timer::ptr timer;
//   std::shared_ptr<timer_info> tinfo(new timer_info);
//   std::weak_ptr<timer_info> winfo(tinfo);

//   if (timeout_ms != (uint64_t)-1) {
//     timer = iom->addConditionTimer(
//         timeout_ms,
//         [winfo, fd, iom]() {
//           auto t = winfo.lock();
//           if (!t || t->cancelled) {
//             return;
//           }
//           t->cancelled = ETIMEDOUT;
//           iom->cancelEvent(fd, sylar::IOManager::WRITE);
//         },
//         winfo);
//   }

//   int rt = iom->addEvent(fd, sylar::IOManager::WRITE);
//   if (rt == 0) {
//     sylar::Fiber::YieldToHold();
//     if (timer) {
//       timer->cancel();
//     }
//     if (tinfo->cancelled) {
//       errno = tinfo->cancelled;
//       return -1;
//     }
//   } else {
//     if (timer) {
//       timer->cancel();
//     }
//     SYLAR_LOG_ERROR(g_logger) << "connect addevent(" << fd << ",WRITE)
//     error";
//   }
//   int error = 0;
//   socklen_t len = sizeof(int);
//   if (-1 == getsockopt_f(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
//     return -1;
//   }
//   if (!error) {
//     return 0;
//   } else {
//     errno = error;
//     return -1;
//   }
// }
int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,
                         uint64_t timeout_ms) {
  if (!sylar::t_hook_enable) {
    return connect_f(fd, addr, addrlen);
  }
  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
  if (!ctx || ctx->isClosed()) {
    errno = EBADF;
    return -1;
  }

  if (!ctx->isSocket()) {
    return connect_f(fd, addr, addrlen);
  }
  // 判断fd是否被显式设置为了非阻塞模式，如果是则调用系统的connect函数并返回。
  if (ctx->getUserNonblock()) {
    return connect_f(fd, addr, addrlen);
  }

  // 调用系统的connect函数，由于套接字是非阻塞的，这里会直接返回EINPROGRESS错误。
  size_t n = connect_f(fd, addr, addrlen);
  if (n == 0) {
    return 0;
  } else if (errno != EINPROGRESS) {
    return n;
  }

  sylar::IOManager *iom = sylar::IOManager::GetThis();
  sylar::Timer::ptr timer;
  // std::shared_ptr<timer_info> tinfo(new timer_info);
  std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();
  std::weak_ptr<timer_info> winfo(tinfo);
  // 如果超时参数有效，则添加一个条件定时器，在定时时间到后通过t->cancelled设置超时标志并触发一次WRITE事件。
  if (timeout_ms != (uint64_t)-1) {
    timer = iom->addConditionTimer(
        timeout_ms,
        [winfo, fd, iom]() {
          auto t = winfo.lock();
          /// 判断tinfo条件是否存在
          /// 或t->cancelled超时的话
          if (!t || t->cancelled) {
            return;
          }
          /// 将条件设为超时状态
          t->cancelled = ETIMEDOUT;
          /// 取消掉可写事件，会执行cb，由于下面注册空回调
          /// 因此会回到该线程
          iom->cancelEvent(fd, sylar::IOManager::WRITE);
        },
        winfo);
  }

  /// 注册可写事件 回调为当前协程
  /// 说明连接成功已经可写
  /// 非阻塞connect的完成被认为是使响应套接字可写
  int rt = iom->addEvent(fd, sylar::IOManager::WRITE);
  if (rt == 0) {
    /// 关键点:切换出去，让出CPU执行权
    sylar::Fiber::YieldToHold();
    if (timer) {
      timer->cancel();
    }
    /// 由于 iom->cancelEvent(fd, pudge::IOManager::WRITE);引起
    /// 则会触发如下的超时
    if (tinfo->cancelled) {
      errno = tinfo->cancelled;
      return -1;
    }
  } else {
    if (timer) {
      timer->cancel();
    }
    SYLAR_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
  }

  int error = 0;
  socklen_t len = sizeof(int);
  if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
    return -1;
  }
  if (!error) {
    return 0;
  } else {
    errno = error;
    return -1;
  }
}
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  return connect_with_timeout(sockfd, addr, addrlen, sylar::s_connect_timeout);
  // return ::connect(sockfd, addr, addrlen);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  int fd = do_io(sockfd, accept_f, "accept", sylar::IOManager::READ,
                 SO_RCVTIMEO, addr, addrlen);
  if (fd >= 0) {
    sylar::FdMgr::GetInstance()->get(fd, true);
  }
  return fd;
}

// read
ssize_t read(int fd, void *buf, size_t count) {
  return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO, buf,
               count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, readv_f, "readv", sylar::IOManager::READ, SO_RCVTIMEO, iov,
               iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  return do_io(sockfd, recv_f, "recv", sylar::IOManager::READ, SO_RCVTIMEO, buf,
               len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
  return do_io(sockfd, recvfrom_f, "recvfrom", sylar::IOManager::READ,
               SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return do_io(sockfd, recvmsg_f, "recvmsg", sylar::IOManager::READ,
               SO_RCVTIMEO, msg, flags);
}

// write
ssize_t write(int fd, const void *buf, size_t count) {
  return do_io(fd, write_f, "write", sylar::IOManager::WRITE, SO_SNDTIMEO, buf,
               count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, writev_f, "writev", sylar::IOManager::WRITE, SO_SNDTIMEO,
               iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  return do_io(sockfd, send_f, "send", sylar::IOManager::WRITE, SO_SNDTIMEO,
               buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
  return do_io(sockfd, sendto_f, "sendto", sylar::IOManager::WRITE, SO_SNDTIMEO,
               buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  return do_io(sockfd, sendmsg_f, "sendmsg", sylar::IOManager::WRITE,
               SO_SNDTIMEO, msg, flags);
}

// close
int close(int fd) {
  if (!sylar::t_hook_enable) {
    return close_f(fd);
  }

  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd, true);
  if (ctx) {
    auto iom = sylar::IOManager::GetThis();
    if (iom) {
      // 取消掉fd上的全部事件
      iom->cancelAll(fd);
      // 删除fd的上下文
      sylar::FdMgr::GetInstance()->del(fd);
    }
  }
  // return close(fd);  // 狗日 浪费老子一晚上
  return close_f(fd);
}

int fcntl(int fd, int cmd, ...) {
  va_list va;
  va_start(va, cmd);
  switch (cmd) {
    case F_SETFL: {
      int arg = va_arg(va, int);
      va_end(va);
      sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
      if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
        return fcntl_f(fd, cmd, arg);
      }
      ctx->setUserNonblock(arg & O_NONBLOCK);
      if (ctx->getSysNonblock()) {
        arg |= O_NONBLOCK;
      } else {
        arg &= ~O_NONBLOCK;
      }
      return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETFL: {
      va_end(va);
      int arg = fcntl_f(fd, cmd);
      sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
      if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
        return arg;
      }
      if (ctx->getUserNonblock()) {
        return arg |= O_NONBLOCK;
      } else {
        return arg &= ~O_NONBLOCK;
      }
    } break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
#ifdef F_SETPIPE_SZ
    case F_SETPIPE_SZ:
#endif
    {
      int arg = va_arg(va, int);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    }

    break;
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
#ifdef F_GETPIPE_SZ
    case F_GETPIPE_SZ:
#endif
    {
      va_end(va);
      return fcntl_f(fd, cmd);
    } break;
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK: {
      struct flock *arg = va_arg(va, struct flock *);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETOWN_EX:
    case F_SETOWN_EX: {
      struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    } break;
    default:
      va_end(va);
      return fcntl_f(fd, cmd);
  }
}

int ioctl(int fd, unsigned long request, ...) {
  va_list va;
  va_start(va, request);
  void *arg = va_arg(va, void *);
  va_end(va);

  if (FIONBIO == request) {
    bool user_nonblock = !!*(int *)arg;
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClosed() || !ctx->isClosed()) {
      return ioctl_f(fd, request, arg);
    }
    ctx->setUserNonblock(user_nonblock);
  }
  return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
  return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
  if (!sylar::t_hook_enable) {
    // return setsockopt(sockfd, level, optname, optval, optlen); // 死循环
    return setsockopt_f(sockfd, level, optname, optval, optlen);
  }

  if (level == SOL_SOCKET) {
    // 这里要特殊处理SO_RECVTIMEO和SO_SNDTIMEO，在应用层记录套接字的读写超时，方便协程调度器获取。
    if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
      sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(sockfd);
      if (ctx) {
        const timeval *tv = (const timeval *)optval;
        ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
      }
    }
  }
  return setsockopt_f(sockfd, level, optname, optval, optlen);
}

#ifdef __cplusplus
}
#endif
