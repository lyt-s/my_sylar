#include "iomanager.h"

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "schedule.h"
#include "thread.h"
namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

IOManager::FdContext::EventContext &IOManager::FdContext::getContext(IOManager::Event event) {
  switch (event) {
    case IOManager::READ:
      return read;
    case IOManager::WRITE:
      return write;
    default:
      SYLAR_ASSERT2(false, "getContext");
  }
  throw std::invalid_argument("getContext invalid event");
}
void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext &ctx) {
  ctx.scheduler = nullptr;
  ctx.fiber.reset();
  ctx.cb = nullptr;
}
void IOManager::FdContext::triggerEvent(IOManager::Event event) {
  // SYLAR_LOG_INFO(g_logger) << "fd=" << fd
  //     << " triggerEvent event=" << event
  //     << " events=" << events;
  SYLAR_ASSERT(events & event);
  // if(SYLAR_UNLIKELY(!(event & event))) {
  //     return;
  // }
  events = (Event)(events & ~event);
  EventContext &ctx = getContext(event);
  if (ctx.cb) {
    ctx.scheduler->schedule(&ctx.cb);
  } else {
    ctx.scheduler->schedule(&ctx.fiber);
  }
  ctx.scheduler = nullptr;
  return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
  // 创建epoll实例
  m_epfd = epoll_create(5000);
  SYLAR_ASSERT(m_epfd > 0);

  // 创建pipe，获取m_tickleFds[2]，其中m_tickleFds[0]是管道的读端，m_tickleFds[1]是管道的写端
  int rt = pipe(m_tickleFds);
  // 不等于0异常
  SYLAR_ASSERT(!rt);

  // 注册pipe读句柄的可读事件，用于tickle调度协程，通过epoll_event.data.fd保存描述符
  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events = EPOLLIN | EPOLLET;  // 边沿触发
  event.data.fd = m_tickleFds[0];    //设置句柄

  // 非阻塞方式，配合边缘触发
  rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
  SYLAR_ASSERT(!rt);

  // 将管道的读描述符加入epoll多路复用，如果管道可读，idle中的epoll_wait会返回
  rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
  SYLAR_ASSERT(!rt);

  // m_fdContexts.resize(64); bug
  contextResize(64);

  // 这里直接开启了Schedluer，也就是说IOManager创建即可调度协程
  start();
}

IOManager::~IOManager() {
  stop();
  close(m_epfd);
  close(m_tickleFds[0]);
  close(m_tickleFds[1]);

  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (m_fdContexts[i]) {
      delete m_fdContexts[i];
    }
  }
}

void IOManager::contextResize(size_t size) {
  m_fdContexts.resize(size);
  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (!m_fdContexts[i]) {
      m_fdContexts[i] = new FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
  FdContext *fd_ctx = nullptr;
  // 读锁
  RWMutexType::ReadLock lock(m_mutex);
  if (static_cast<int>(m_fdContexts.size()) > fd) {
    // 优化 直接在if中加读锁
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  } else {
    lock.unlock();
    RWMutexType::WriteLock lock2(m_mutex);
    contextResize(fd * 1.5);
    fd_ctx = m_fdContexts[fd];
  }
  // 同一个fd不允许重复添加相同的事件
  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if (fd_ctx->events & event) {
    SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd << "event= " << event
                              << "fd_ctx->events=" << fd_ctx->events;
    SYLAR_ASSERT(!(fd_ctx->events & event));
  }

  // 将新的事件加入epoll_wait，使用epoll_event的私有指针存储FdContext的位置
  int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  epevent.events = EPOLLET | fd_ctx->events | event;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "," << op << ", " << fd << ","
                              << epevent.events << "):" << rt << "(" << errno << ") ("
                              << strerror(errno) << ")";
    return -1;
  }

  ++m_pendingEventCount;

  // 找到这个fd的event事件对应的EventContext，对其中的scheduler, cb, fiber进行赋值
  fd_ctx->events = (Event)(fd_ctx->events | event);
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

  // 赋值scheduler和回调函数，如果回调函数为空，则把当前协程当成回调执行体
  event_ctx.scheduler = Scheduler::GetThis();
  if (cb) {
    event_ctx.cb.swap(cb);
  } else {
    event_ctx.fiber = Fiber::GetThis();
    SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC,
                  "state=" << event_ctx.fiber->getState());
  }
  return 0;
}

/**
 * @brief 删除事件
 * @param[in] fd socket句柄
 * @param[in] event 事件类型
 * @attention 不会触发事件
 * @return 是否删除成功
 */
bool IOManager::delEvent(int fd, Event event) {
  RWMutexType::ReadLock lock(m_mutex);
  if (static_cast<int>(m_fdContexts.size()) <= fd) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();
  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if (!(fd_ctx->events & event)) {
    return false;
  }

  // 清除指定的事件，表示不关心这个事件了，如果清除之后结果为0，则从epoll_wait中删除该文件描述符
  Event new_events = (Event)(fd_ctx->events & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "," << op << ", " << fd << ","
                              << epevent.events << "):" << rt << "(" << errno << ") ("
                              << strerror(errno) << ")";
    return false;
  }

  --m_pendingEventCount;
  // 重置该fd对应的event事件上下文
  fd_ctx->events = new_events;
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx);
  return true;
}

/**
 * @brief 取消事件
 * @param[in] fd socket句柄
 * @param[in] event 事件类型
 * @attention 如果该事件被注册过回调，那就触发一次回调事件
 * @return 是否删除成功
 */
bool IOManager::cancelEvent(int fd, Event event) {
  RWMutexType::ReadLock lock(m_mutex);
  if (static_cast<int>(m_fdContexts.size()) <= fd) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if (!(fd_ctx->events & event)) {
    return false;
  }
  // 删除事件 todo
  Event new_events = (Event)(fd_ctx->events & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "," << op << ", " << fd << ","
                              << epevent.events << "):" << rt << "(" << errno << ") ("
                              << strerror(errno) << ")";
    return false;
  }

  // 删除之前触发一次事件
  fd_ctx->triggerEvent(event);
  --m_pendingEventCount;
  return true;
}

/**
 * @brief 取消所有事件
 * @details 所有被注册的回调事件在cancel之前都会被执行一次
 * @param[in] fd socket句柄
 * @return 是否删除成功
 */
bool IOManager::cancelAll(int fd) {
  RWMutexType::ReadLock lock(m_mutex);
  if (static_cast<int>(m_fdContexts.size()) <= fd) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if (!(fd_ctx->events)) {
    return false;
  }

  // 删除全部事件
  int op = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = 0;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "," << op << ", " << fd << ","
                              << epevent.events << "):" << rt << "(" << errno << ") ("
                              << strerror(errno) << ")";
    return false;
  }

  // 触发全部已注册的事件
  if (fd_ctx->events & READ) {
    fd_ctx->triggerEvent(READ);
    --m_pendingEventCount;
  }
  if (fd_ctx->events & WRITE) {
    fd_ctx->triggerEvent(WRITE);
    --m_pendingEventCount;
  }
  SYLAR_ASSERT(fd_ctx->events == 0);
  return true;
}

IOManager *IOManager::GetThis() { return dynamic_cast<IOManager *>(Scheduler::GetThis()); }

/**
 * @brief 通知调度器有任务要调度
 * @details 写pipe让idle协程从epoll_wait退出，待idle协程yield之后Scheduler::run就可以调度其他任务
 * 如果当前没有空闲调度线程，那就没必要发通知
 */
void IOManager::tickle() {
  // 如果没有空闲线程，就直接返回
  if (!hasIdleThreads()) {
    return;
  }
  // 往管道的写端写
  int rt = write(m_tickleFds[1], "T", 1);
  SYLAR_ASSERT(rt == 1);
}
bool IOManager::stopping(uint64_t &timeout) {
  timeout = getNextTimer();
  return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}
bool IOManager::stopping() {
  uint64_t timeout = 0;
  return stopping(timeout);
}

/**
 * @brief idle协程
 * @details
 * 对于IO协程调度来说，应阻塞在等待IO事件上，idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件就绪
 * 调度器无调度任务时会阻塞idle协程上，对IO调度器而言，idle状态应该关注两件事，一是有没有新的调度任务，对应Schduler::schedule()，
 * 如果有新的调度任务，那应该立即退出idle状态，并执行对应的任务；二是关注当前注册的所有IO事件有没有触发，如果有触发，那么应该执行
 * IO事件对应的回调函数
 */
void IOManager::idle() {
  // 一次epoll_wait最多检测256个就绪事件，如果就绪事件超过了这个数，那么会在下轮epoll_wati继续处理
  const uint64_t MAX_EVNETS = 256;
  epoll_event *events = new epoll_event[MAX_EVNETS]();
  std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr) {
    // delete[] events; //error
    // 传进来的是ptr
    delete[] ptr;
  });

  while (true) {
    uint64_t next_timeout = 0;
    if (stopping(next_timeout)) {
      // //获得当前的时间
      // next_timeout = getNextTimer();
      // if (next_timeout == ~0ull) {
      //   SYLAR_LOG_INFO(g_logger) << "name=" << getName() << "idle stopping exit";
      //   break;
      // }
      SYLAR_LOG_INFO(g_logger) << "name=" << getName() << "idle stopping exit";
      break;
    }

    int rt = 0;
    do {
      static const int MAX_TIMEOUT = 3000;
      if (next_timeout != ~0ull) {
        next_timeout = static_cast<int>(next_timeout) > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
      } else {
        next_timeout = MAX_TIMEOUT;
      }
      rt = epoll_wait(m_epfd, events, MAX_EVNETS, static_cast<int>(next_timeout));

      if (rt < 0 && errno == EINTR) {
      } else {
        break;
      }
    } while (true);

    std::vector<std::function<void()>> cbs;
    listExpiredCb(cbs);

    if (!cbs.empty()) {
      // SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
      schedule(cbs.begin(), cbs.end());
      cbs.clear();
    }

    // 遍历所有发生的事件，根据epoll_event的私有指针找到对应的FdContext，进行事件处理
    for (int i = 0; i < rt; ++i) {
      epoll_event &event = events[i];
      // SYLAR_LOG_INFO(g_logger) << event.data.fd;
      if (event.data.fd == m_tickleFds[0]) {
        // ticklefd[0]用于通知协程调度，这时只需要把管道里的内容读完即可，本轮idle结束Scheduler::run会重新执行协程调度
        u_int8_t dummy[256];

        // 一个一个的读
        while (read(m_tickleFds[0], &dummy, sizeof(dummy)) > 0)

          ;

        continue;
      }
      // 通过epoll_event的私有指针获取FdContext
      FdContext *fd_ctx = static_cast<FdContext *>(event.data.ptr);
      FdContext::MutexType::Lock lock(fd_ctx->mutex);
      if (event.events & (EPOLLERR | EPOLLHUP)) {
        event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
        ;
      }
      int real_events = NONE;
      if (event.events & EPOLLIN) {
        real_events |= READ;
      }
      if (event.events & EPOLLOUT) {
        real_events |= WRITE;
      }

      if ((fd_ctx->events & real_events) == NONE) {
        continue;
      }

      // 剩余事件
      int left_events = (fd_ctx->events & ~real_events);
      int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = EPOLLET | left_events;

      int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
      if (rt2) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "," << op << ", " << fd_ctx->fd
                                  << "," << event.events << "):" << rt2 << "(" << errno << ") ("
                                  << strerror(errno) << ")";
      }

      if (real_events & READ) {
        // 有trigger 数量减一
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
      }

      if (real_events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
      }
    }

    Fiber::ptr cur = Fiber::GetThis();
    auto raw_ptr = cur.get();
    cur.reset();

    // 切换到调度协程，执行任务
    raw_ptr->swapOut();
  }
}

void IOManager::onTimerInsertedAtFront() { tickle(); }
}  // namespace sylar
