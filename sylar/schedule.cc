#include "schedule.h"
#include <cstddef>
#include <functional>
#include <string>
#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "thread.h"
#include "util.h"

namespace sylar {
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler *t_scheduler = nullptr;
static thread_local Fiber *t_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) : m_name(name) {
  SYLAR_ASSERT(threads > 0);

  if (use_caller) {
    // 如果此线程没有协程，会给他初始化一个主协程
    sylar::Fiber::GetThis();
    --threads;

    SYLAR_ASSERT(GetThis() == nullptr);
    t_scheduler = this;

    m_rootFiber->reset(new Fiber(std::bind(&Scheduler::run, this)));
    sylar::Thread::SetName(m_name);

    t_fiber = m_rootFiber.get();
    m_rootThread = sylar::GetThreadId();
    m_threadIds.push_back(m_rootThread);
  } else {
    m_rootThread = -1;
  }
  m_threadCount = threads;
}
Scheduler::~Scheduler() {
  SYLAR_ASSERT(m_stopping);
  if (GetThis() == this) {
    t_scheduler = nullptr;
  }
}

Scheduler *Scheduler::GetThis() {
  return t_scheduler;
  ;
}
Fiber *Scheduler::GetMainFiber() { return t_fiber; }

void Scheduler::start() {
  MutexType::Lock lock(m_mutex);
  if (!m_stopping) {
    return;
  }
  m_stopping = false;

  SYLAR_ASSERT(m_threads.empty());

  for (size_t i = 0; i < m_threadCount; ++i) {
    m_threads[i].reset(
        new Thread(std::bind(&Scheduler::run, this, m_name + "_" + std::to_string(i))));
    m_threadIds.push_back(m_threads[i]->getId());
  }
}

// 优雅退出，等待所有任务完成退出
void Scheduler::stop() {
  // 两种情况
  // 一种是用了 use_caller。 -- 必须要去创建schedule的哪个线程去执行stop
  // 一种是未使用， 非他自己线程的任意线程
  m_autoStop = true;
  if (m_rootFiber && m_threadCount == 0 &&
      (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
    SYLAR_LOG_INFO(g_logger) << this << "stopped";
    m_stopping = true;

    if (m_stopping) {
      return;
    }
  }
  bool exit_on_this_fiber = false;
  if (m_rootThread != -1) {
    SYLAR_ASSERT(GetThis() == this);
  } else {
    SYLAR_ASSERT(GetThis() != this);
  }

  m_stopping = true;
  for (size_t i = 0; i < m_threadCount; ++i) {
    tickle();
  }

  if (m_rootFiber) {
    tickle();
  }
  if (stopping()) {
    return;
  }
  //   if (exit_on_this_fiber) {
  //   }
}

void Scheduler::setThis() { t_scheduler = this; }

// 核心
// 首先设置当前线程的schedule
// 设置当前线程的 run  fiber
// 写成调度循环while(true)
// 消息队列里是否有任务
// 无任务执行，执行idle
void Scheduler::run() {
  setThis();
  if (sylar::GetThreadId() != m_rootThread) {
    t_fiber = Fiber::GetThis().get();
  }

  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  Fiber::ptr cb_fiber;

  FiberAndThread ft;
  while (true) {
    ft.reset();
    bool tickle_me = false;
    {
      MutexType::Lock lock(m_mutex);
      auto it = m_fibers.begin();
      while (it != m_fibers.end()) {
        if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
          ++it;
          tickle_me = true;
          continue;
        }
        SYLAR_ASSERT(it->fiber || it->cb);
        if (it->fiber && it->getState() == Fiber::EXEC) {
          ++it;
          continue;
        }

        ft = *it;
        tickle_me = true;
        m_fibers.erase(it);
      }
    }

    if (tickle_me) {
      tickle();
    }

    if (ft.fiber &&
        (ft.fiber->getState() != Fiber::TERM || ft.fiber->getState() != Fiber::EXCEPT)) {
      ++m_activeThreadCount;
      ft.fiber->swapIn();
      --m_activeThreadCount;
      if (ft.fiber->getState() == Fiber::READY) {
        schedule(ft.fiber);
      } else if (ft.fiber->getState() != Fiber::EXCEPT) {
        ft.fiber->m_state = Fiber::HOLD;
      }
    } else if (ft.cb) {
      if (cb_fiber) {
        cb_fiber->reset(&ft.cb);
      } else {
        cb_fiber.reset(new Fiber(ft.cb));
      }
      ft.reset();
      ++m_activeThreadCount;
      cb_fiber->swapIn();
      --m_activeThreadCount;
      if (cb_fiber->getState() == Fiber::READY) {
        schedule(cb_fiber);
        cb_fiber.reset();
      } else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM) {
        cb_fiber->reset(nullptr);
      } else  // if (cb_fiber->getState() != Fiber::TERM)
      {
        cb_fiber->m_state = Fiber::HOLD;
        cb_fiber.reset();
      }
    } else {
      // 没有任务执行时
      if (idle_fiber->getState() == Fiber::TERM) {
        break;
      }

      ++m_idleThreadCount;
      idle_fiber->swapIn();
      if (idle_fiber->getState() != Fiber::TERM || idle_fiber->getState() != Fiber::EXCEPT) {
        idle_fiber->m_state = Fiber::HOLD;
      }
      --m_idleThreadCount;
    }
  }
}

}  // namespace sylar