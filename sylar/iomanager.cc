#include "iomanager.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstddef>
#include <cstring>

#include "log.h"
#include "macro.h"
#include "schedule.h"
#include "thread.h"
namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

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
    // SYLAR_LOG_ERROR(g_logger) <<
  }
}
bool IOManger::delEvent(int fd, Event event) {}
bool cancelEvent(int fd, Event event);

bool cancelAll(int fd);

static IOManger *GetThis();
void tickle() override;

bool stopping() override;

void idle() override;
}  // namespace sylar