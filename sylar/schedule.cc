#include "schedule.h"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "thread.h"
#include "util.h"

namespace sylar {

// 库的函数功能都放到system中
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 线程局部变量，调度器
static thread_local Scheduler *t_scheduler = nullptr;

// 主协程函数 --->保存当前线程的调度协程
static thread_local Fiber *t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) : m_name(name) {
  // 首先输入的线程要大于0
  SYLAR_ASSERT(threads > 0);

  if (use_caller) {
    // 如果此线程没有协程，会给他初始化一个主协程
    sylar::Fiber::GetThis();
    // 因为use_caller,表示当前线程也是调度线程之一。
    --threads;

    // 防止此线程创建多个协程调度器。这里的GetThis() 为
    // Scheduler *Scheduler::GetThis() { return t_scheduler; }
    SYLAR_ASSERT(Scheduler::GetThis() == nullptr);
    t_scheduler = this;

    // m_rootFiber是调度协程
    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
    sylar::Thread::SetName(m_name);  // 修改线程名称

    // 在一个线程内声明一个调度器，在将当前线程放入调度器内，他的主协程就不是线程的主协程
    // ，而是执行run方法的主协程 主协程。 ？？？ todo
    t_scheduler_fiber = m_rootFiber.get();
    m_rootThread = sylar::GetThreadId();
    m_threadIds.push_back(m_rootThread);
  } else {
    m_rootThread = -1;
  }
  m_threadCount = threads;
}

Scheduler::~Scheduler() {
  // 是否正在停止
  SYLAR_ASSERT(m_stopping);
  if (GetThis() == this) {
    t_scheduler = nullptr;
  }
}

// 用于获取当前线程的调度器的调度协程
// static thread_local Scheduler *t_scheduler = nullptr;
Scheduler *Scheduler::GetThis() { return t_scheduler; }

// 主协程 static thread_local Fiber *t_scheduler_fiber = nullptr;
Fiber *Scheduler::GetMainFiber() { return t_scheduler_fiber; }

// 启动线程池
void Scheduler::start() {
  MutexType::Lock lock(m_mutex);
  // m_stopping 默认为true --->表示未启动， false 则为启动
  if (!m_stopping) {
    return;
  }
  m_stopping = false;
  // 此时线程池应该是空的，未分配线程。
  SYLAR_ASSERT(m_threads.empty());

  // 此时配置线程池，线程都跑run方法，
  m_threads.resize(m_threadCount);
  for (size_t i = 0; i < m_threadCount; ++i) {
    m_threads[i].reset(
        new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
    // 线程构造函数放置信号量，确保线程跑起来，才能真正拿到线程id。
    m_threadIds.push_back(m_threads[i]->getId());
  }
  lock.unlock();

  // if (m_rootFiber) {
  //   // m_rootFiber->swapIn();
  //  切换到run方法中去
  //   m_rootFiber->call();
  //   SYLAR_LOG_INFO(g_logger) << "call out";
  // }
}

// 优雅退出，等待所有任务完成退出
// 线程没事做时，需要循环等待，stop就是做一个等的动作
void Scheduler::stop() {
  // 两种情况
  // 一种是用了 use_caller。 -- 必须要去创建schedule的哪个线程去执行stop
  // 一种是未使用， 对应额外创建一个线程进行协程调度、main函数线程不参与调度的情况。
  m_autoStop = true;
  // m_rootFiber是指创建schedule方法的线程中的协程的里面的执行run的那个协程-->调度协程;
  // TERM --->结束状态, INIT---> 初始化状态
  if (m_rootFiber && m_threadCount == 0 &&
      (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
    SYLAR_LOG_INFO(g_logger) << this << "stopped";
    // 此时已结束，或者未跑起来，那么我们就要设置结束
    m_stopping = true;
    // use_caller 并且只有一个线程的时候，从这里返回
    if (m_stopping) {
      return;
    }
  }

  // m_rootThread != -1，说明是 use_caller 线程
  if (m_rootThread != -1) {
    // 目的是，把创建它的哪个线程也使用上时，stop一定要在创建他的那个线程执行
    SYLAR_ASSERT(Scheduler::GetThis() == this);
  } else {
    SYLAR_ASSERT(GetThis() != this);
  }

  // 设置为true 表示可以结束
  m_stopping = true;
  for (size_t i = 0; i < m_threadCount; ++i) {
    // 使线程唤醒
    tickle();
  }

  // 有多少run 运行多少个tickle
  // 主线程 tickle完， 返回
  if (m_rootFiber) {
    tickle();
  }

  // 协程调度器使用当前线程
  if (m_rootFiber) {
    // while (!stopping()) {
    //   if (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::EXCEPT) {
    //     m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
    //     SYLAR_LOG_INFO(g_logger) << "root fiber is term, reset";
    //     // bug
    //     t_scheduler_fiber = m_rootFiber.get();
    //   }
    //   m_rootFiber->call();
    if (!stopping()) {
      m_rootFiber->call();
    }
  }

  std::vector<Thread::ptr> thrs;
  {
    MutexType::Lock lock(m_mutex);
    thrs.swap(m_threads);
  }

  // 所有线程 join
  for (auto &i : thrs) {
    i->join();
  }
}

void Scheduler::setThis() { t_scheduler = this; }
void Scheduler::tickle() { SYLAR_LOG_INFO(g_logger) << "tickle"; }

bool Scheduler::stopping() {
  MutexType::Lock lock(m_mutex);
  return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

// 没有任务时，执行idle
void Scheduler::idle() {
  SYLAR_LOG_INFO(g_logger) << "idle";
  while (!stopping()) {
    sylar::Fiber::YieldToHold();
  }
}
// 核心--->协程和线程的关系
// 首先设置当前线程的schedule
// 设置当前线程的 run  fiber
// 写成调度循环while(true)
//   消息队列里是否有任务
//   无任务执行，执行idle

// 这个run 一个线程只会有一个协程来执行
// 一个是use_caller的线程在run执行,线程池里面自己创建的线程也在run中执行
void Scheduler::run() {
  // 设置当前的协程调度器
  setThis();
  // 如果当前线程id != 主协程id
  if (sylar::GetThreadId() != m_rootThread) {
    // 就将主fiber 设置为当前线程id
    t_scheduler_fiber = Fiber::GetThis().get();
  }

  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  Fiber::ptr cb_fiber;

  // 协程和线程
  FiberAndThread ft;
  while (true) {
    ft.reset();
    bool tickle_me = false;
    // 防止协程调度结束
    bool is_active = false;
    // 协程的消息队列中，取出一个协程任务
    {
      MutexType::Lock lock(m_mutex);
      auto it = m_fibers.begin();
      while (it != m_fibers.end()) {
        // 如果一个任务(协程)已经指定好在哪个线程执行，我当前执行run的线程id != 他期望的线程id
        if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
          ++it;
          // 通知别人来处理，后面可以优化，通知这里。
          tickle_me = true;
          continue;
        }
        // fiber cb 至少要有一个，确定任务非空
        SYLAR_ASSERT(it->fiber || it->cb);
        // exec--正在执行状态，此时也不处理。
        if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
          ++it;
          continue;
        }

        // 开始处理
        ft = *it;
        tickle_me = true;
        m_fibers.erase(it++);  // 从待处理队列中删除，这里注意迭代器删除的细节，当心bug
        ++m_activeThreadCount;
        is_active = true;
        break;  // while 只循环一次，目的是依次处理每个待处理的任务。
      }
      // tickle_me |= it != m_fibers.end(); todo
    }

    //取出了一个需要执行任务，唤醒其他线程
    if (tickle_me) {
      tickle();  //
    }

    // 执行这个协程，fiber的方式
    if (ft.fiber &&
        (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
      // 执行 ft，唤醒，去执行
      ft.fiber->swapIn();
      // 执行完之后，数量--
      --m_activeThreadCount;
      // 判断是否需要再次执行
      if (ft.fiber->getState() == Fiber::READY) {
        schedule(ft.fiber);
      } else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
        // 设置暂停状态，除了上面的判断的语句外，除了ready， 就是hold状态？？
        ft.fiber->m_state = Fiber::HOLD;
      }
      // FiberAndThread::reset
      ft.reset();
    }
    // 执行这个协程，cb 的方式
    else if (ft.cb) {
      if (cb_fiber) {
        // 重置协程状态，
        cb_fiber->reset(ft.cb);
      } else {
        // 智能指针的 reset
        cb_fiber.reset(new Fiber(ft.cb));
      }
      // 释放掉
      ft.reset();
      // 唤醒cb_fiber
      cb_fiber->swapIn();
      // 运行结束后
      --m_activeThreadCount;
      // 判断是否需要再次执行
      if (cb_fiber->getState() == Fiber::READY) {
        schedule(cb_fiber);
        cb_fiber.reset();  // 智能指针reset
      } else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM) {
        cb_fiber->reset(
            nullptr);  // 出现异常或者执行完成，智能指针的reset，释放掉，并没有引起他的析构
      } else  // if (cb_fiber->getState() != Fiber::TERM)
      {
        // 没有结束，设置暂停状态
        cb_fiber->m_state = Fiber::HOLD;
        cb_fiber.reset();
      }
    }
    // 调度协程，后面这里表示没有任务执行，事情做完之后
    else {
      if (is_active) {
        --m_activeThreadCount;
        continue;
      }
      // fiber已经结束，直接break
      if (idle_fiber->getState() == Fiber::TERM) {
        SYLAR_LOG_INFO(g_logger) << "idle fiber term";
        break;
      }

      // 空闲线程数 ++
      ++m_idleThreadCount;
      // idle_fiber 去执行，---***每个线程，同一时刻只有一个协程在执行。
      // ？？在执行idle_fiber时，来了任务要怎么唤醒
      idle_fiber->swapIn();
      --m_idleThreadCount;
      // if (idle_fiber->getState() != Fiber::TERM || idle_fiber->getState() != Fiber::EXCEPT) //
      // ---> bug
      // idle_fiber执行完之后，判断状态来进行处理，
      if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
        idle_fiber->m_state = Fiber::HOLD;
      }
    }
  }
}

}  // namespace sylar