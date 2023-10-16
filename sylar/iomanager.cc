#include "iomanager.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstddef>
#include <cstring>

#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "schedule.h"
#include "thread.h"
namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

IOManger::FdContext::EventContext &IOManger::FdContext::getContext(IOManger::Event event) {
  switch (event) {
    case IOManger::READ:
      return read;
    case IOManger::WRITE:
      return write;
    default:
      SYLAR_ASSERT2(false, "getcontext");
  }
}
void IOManger::FdContext::resetContext(IOManger::FdContext::EventContext &ctx) {
  ctx.scheduler = nullptr;
  ctx.fiber->reset();
  ctx.cb = nullptr;
}
void IOManger::FdContext::triggerEvent(IOManger::Event event) {
  SYLAR_ASSERT(m_events & event);
  m_events = (Event)(m_events & ~event);
  EventContext &ctx = getContext(event);

  if (ctx.cb) {
    ctx.scheduler->schedule(&ctx.cb);
  } else {
    ctx.scheduler->schedule(&ctx.fiber);
  }

  ctx.scheduler = nullptr;
  return;
}

IOManger::IOManger(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
  m_epfd = epoll_create(5000);
  SYLAR_ASSERT(m_epfd > 0);

  // pipe ??
  int rt = pipe(m_tickleFds);
  SYLAR_ASSERT(rt);

  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events = EPOLLIN | EPOLLET;  // 边沿触发
  event.data.fd = m_tickleFds[0];    //设置句柄

  rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
  SYLAR_ASSERT(rt);

  rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
  SYLAR_ASSERT(rt);

  m_fdContexts.resize(64);

  start();
}

IOManger::~IOManger() {
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

void IOManger::contextResize(size_t size) {
  m_fdContexts.resize(size);
  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (!m_fdContexts[i]) {
      m_fdContexts[i] = new FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

int IOManger::addEvent(int fd, Event event, std::function<void()> cb) {
  FdContext *fd_ctx = nullptr;
  RWMutexType::ReadLock lock(m_mutex);
  if (m_fdContexts.size() > fd) {
    // 优化 直接在if中加读锁
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  } else {
    lock.unlock();
    RWMutexType::WriteLock lock2(m_mutex);
    contextResize(m_fdContexts.size() * 1.5);
    fd_ctx = m_fdContexts[fd];
  }

  FdContext::MutexType::Lock lock(fd_ctx->m_mutex);
  if (fd_ctx->m_events & event) {
    SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd << "event= " << event
                              << "fd_ctx->m_events=" << fd_ctx->m_events;
    SYLAR_ASSERT(!(fd_ctx->m_events & event));
  }
  int op = fd_ctx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  epevent.events = EPOLLET | fd_ctx->m_events | event;
  epevent.data.fd = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "," << op << ", " << fd << ","
                              << epevent.events << "):" << rt << "(" << errno << ") ("
                              << strerror(errno) << ")";
    return -1;
  }

  ++m_pendingEventCount;
  fd_ctx->m_events = (Event)(fd_ctx->m_events | event);
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

  event_ctx.scheduler = Scheduler::GetThis();
  if (cb) {
    event_ctx.cb.swap(cb);
  } else {
    event_ctx.fiber = Fiber::GetThis();
    SYLAR_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
  }
}

bool IOManger::delEvent(int fd, Event event) {
  RWMutexType::ReadLock lock(m_mutex);
  if (m_fdContexts.size() <= fd) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();
  // 重名了
  FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
  if (!(fd_ctx->m_events * event)) {
    return false;
  }
  //??
  Event new_events = (Event)(fd_ctx->m_events & ~event);
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
  fd_ctx->m_events = new_events;
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx);
  return true;
}

bool IOManger::cancelEvent(int fd, Event event) {
  RWMutexType::ReadLock lock(m_mutex);
  if (m_fdContexts.size() <= fd) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
  if (!(fd_ctx->m_events & event)) {
    return false;
  }
  //??
  Event new_events = (Event)(fd_ctx->m_events & ~event);
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

  fd_ctx->triggerEvent(event);
  --m_pendingEventCount;
  return true;
}

bool IOManger::cancelAll(int fd) {
  RWMutexType::ReadLock lock(m_mutex);
  if (m_fdContexts.size() <= fd) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
  if (!(fd_ctx->m_events)) {
    return false;
  }

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

  if (fd_ctx->m_events & READ) {
    fd_ctx->triggerEvent(READ);
    --m_pendingEventCount;
  }
  if (fd_ctx->m_events & WRITE) {
    fd_ctx->triggerEvent(WRITE);
    --m_pendingEventCount;
  }
  SYLAR_ASSERT(fd_ctx->m_events == 0);
  return true;
}

IOManger *GetThis() { return dynamic_cast<IOManger *>(Scheduler::GetThis()); }

void tickle() override;

bool stopping() override;

void idle() override;
}  // namespace sylar