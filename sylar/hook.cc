#include "sylar/hook.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <cstdarg>
#include <memory>

#include "sylar/config.h"
#include "sylar/fd_manager.h"
#include "sylar/fiber.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/macro.h"

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
  // 没有hook直接返回原函数
  if (!sylar::t_hook_enable) {
    //  read(int fd, void *buf, size_t count)
    //  return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO,
    //  buf, count);
    return fun(fd, std::forward<Args>(args)...);
  }

  SYLAR_LOG_DEBUG(g_logger) << "do_io <" << hook_fun_name << ">";
  // 得到fd 对应的 FdCtx
  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
  // 不存在则不是socket, 按照原来方式，不管他
  if (!ctx) {
    return fun(fd, std::forward<Args>(args)...);
  }
  // 存在，且已经关闭了，改errno
  if (ctx->isClosed()) {
    // Bad file descriptor
    errno = EBADF;
    return -1;
  }

  // 如果不是socket或者用户已经设置了非阻塞，那我们也不管
  if (!ctx->isSocket() || ctx->getUserNonblock()) {
    return fun(fd, std::forward<Args>(args)...);
  }

  uint64_t to = ctx->getTimeout(timeout_so);
  // 创建条件
  std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();

retry:
  // 执行原始方法，异步io
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  // EINTR ---Interrupted system call
  // 如果读到了，就可以直接返回出去--->return n;
  // 如果没有读到，这种情况下，重试原始方法
  while (n == -1 && errno == EINTR) {
    n = fun(fd, std::forward<Args>(args)...);
  }
  // 这种是阻塞状态,需要做异步操作
  if (n == -1 && errno == EAGAIN) {
    SYLAR_LOG_DEBUG(g_logger) << "do_io" << hook_fun_name << ">";
    sylar::IOManager *iom = sylar::IOManager::GetThis();
    sylar::Timer::ptr timer;
    // 条件变量，
    std::weak_ptr<timer_info> winfo(tinfo);

    // (uint64_t)-1 = 18446744073709551615 (0xffffffffffffffff)
    // 就是设置了超时时间的情况时，超时器，超时时间
    if (to != (uint64_t)-1) {
      // 等待设置的时间，还没来的话，触发定时器回调，
      timer = iom->addConditionTimer(
          to,
          [winfo, fd, iom, event]() {
            // 定时器回来时，要lock todo --->shared_ptr lock
            auto t = winfo.lock();
            // 如果没有t，或者取消了
            if (!t || t->cancelled) {
              return;
            }
            t->cancelled = ETIMEDOUT;
            // 取消事件，强制唤醒 ,到达时间后，强制唤醒
            iom->cancelEvent(fd, (sylar::IOManager::Event)(event));
          },
          winfo);
    }

    /// 可读可写事件，即可获取协程唤醒的标志，这里默认以当前协程，作为回调函数
    int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));
    // 失败的情况，直接返回。
    if (SYLAR_UNLIKELY(rt)) {
      SYLAR_LOG_ERROR(g_logger)
          << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
      //  定时器添加失败
      if (timer) {
        timer->cancel();
      }
      return -1;

    } else {
      // 成功
      // SYLAR_LOG_ERROR(g_logger) << "do_io<" << hook_fun_name << ">";
      // 让出当前协程的执行时间
      sylar::Fiber::YieldToHold();
      // SYLAR_LOG_ERROR(g_logger) << "do_io<" << hook_fun_name << ">";
      // 有定时器的话，timer，则取消掉
      // 两种情况到这里
      // 1. 真的有事件发生了
      // 2. 设置的定时器超时了，
      if (timer) {
        timer->cancel();
      }
      // 去掉的里面有cancelld
      // 的话，说明是通过定时器超时将这里唤醒的，这设置errno
      if (tinfo->cancelled) {
        errno = tinfo->cancelled;
        return -1;
      }
      // 否则说明有io时间返回，那我们重新读
      goto retry;  // todo
    }
  }
  // 读到数据，返回n
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
      // 模板方法进行bind时，要声明他的bind类型，还有默认参数要写进去，否则会显示没有此函数。
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
int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,
                         uint64_t timeout_ms) {
  // 没有hook
  if (!sylar::t_hook_enable) {
    return connect_f(fd, addr, addrlen);
  }
  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
  // 文件句柄， 不存在或者关闭
  if (!ctx || ctx->isClosed()) {
    errno = EBADF;
    return -1;
  }
  // 不是socket
  if (!ctx->isSocket()) {
    return connect_f(fd, addr, addrlen);
  }
  // 判断fd是否被用户显式设置为了非阻塞模式，如果是则调用系统的connect函数并返回。
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
  /// 连接成功说明已经可写
  /// 非阻塞connect的完成被认为是使响应套接字可写
  int rt = iom->addEvent(fd, sylar::IOManager::WRITE);
  // 这addEvent成功
  if (rt == 0) {
    /// 关键点:切换出去，让出CPU执行权
    // 在这里唤醒时，要么连接成功，要么超时
    sylar::Fiber::YieldToHold();
    // 定时器取消
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
    // 失败时，有定时器的话，取消掉
    if (timer) {
      timer->cancel();
    }
    SYLAR_LOG_ERROR(g_logger) << "connect addevent(" << fd << ",WRITE) error";
  }
  // connect 可写时，就表明连接成功，不需要做其他事情。
  int error = 0;
  socklen_t len = sizeof(int);
  // 取出状态，检查是否有error
  if (-1 == getsockopt_f(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
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
  // return close(fd);// bug
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
