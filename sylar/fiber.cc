#include "fiber.h"

#include <sys/types.h>
#include <ucontext.h>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>

#include "config.h"
#include "log.h"
#include "macro.h"
#include "schedule.h"
#include "thread.h"
#include "util.h"
namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");
/// 全局静态变量，用于生成协程id
static std::atomic<u_int64_t> s_fiber_id{0};
/// 全局静态变量，用于统计当前的协程数
static std::atomic<u_int64_t> s_fiber_count{0};

/// 线程局部变量，每个线程独有一份，当前线程正在运行的协程
static thread_local Fiber *t_fiber = nullptr;

/// 线程局部变量，每个线程独有一份，当前线程的主协程，切换到这个协程，就相当于切换到了主协程中运行，智能指针形式
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<u_int32_t>::ptr g_fiber_stack_size =
    Config::Lookup<u_int32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

class MallocStackAllocator {
 public:
  static void *Alloc(size_t size) { return malloc(size); }

  static void Dealloc(void *vp, size_t size) { return free(vp); }
};

using StackAllocator = MallocStackAllocator;

u_int64_t Fiber::GetFiberId() {
  if (t_fiber) {
    return t_fiber->getId();
  }
  return 0;
}

//
Fiber::Fiber() {
  m_state = EXEC;
  // 设置当前正在运行的协程
  SetThis(this);

  if (getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }

  ++s_fiber_count;

  SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb) {
  ++s_fiber_count;
  m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

  m_stack = StackAllocator::Alloc(m_stacksize);
  if (getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  if (!use_caller) {
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
  } else {
    makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
  }

  SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}
Fiber::~Fiber() {
  --s_fiber_count;
  if (m_stack) {
    SYLAR_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);

    StackAllocator::Dealloc(m_stack, m_stacksize);
  } else {
    SYLAR_ASSERT(!m_cb);
    SYLAR_ASSERT(m_state == EXEC);
    Fiber *cur = t_fiber;
    if (cur == this) {
      SetThis(nullptr);
    }
  }
  SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id= " << m_id;
}

// 重置协程函数，并重置状态
//  INIT，TERM, EXCEPT
void Fiber::reset(std::function<void()> cb) {
  SYLAR_ASSERT(m_stack);
  SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
  m_cb = cb;
  if (getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext(&m_ctx, &Fiber::MainFunc, 0);
  m_state = INIT;
}

// 将执行run的线程换出，换为main函数中的主协程执行
void Fiber::call() {
  // bug
  SetThis(this);
  m_state = EXEC;
  // 和swapIn 的区别---> main函数中的主协程-->t_threadFiber
  if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

void Fiber::back() {
  SetThis(t_threadFiber.get());

  if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

// 切换到当前协程执行
void Fiber::swapIn() {
  SetThis(this);
  SYLAR_ASSERT(m_state != EXEC);
  m_state = EXEC;
  // 取当前协程的主协程，自己swap自己，会死锁，所以建立了call
  // 这里的GetMainFiber--->是指向run函数的那个fiber
  if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

// 把当前协程切换到后台,调度协程换出来
void Fiber::swapOut() {
  SetThis(Scheduler::GetMainFiber());
  if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

// 设置当前协程
void Fiber::SetThis(Fiber *f) { t_fiber = f; }

/**
 * @brief 返回当前线程正在执行的协程
 * @details 如果当前线程还未创建协程，则创建线程的第一个协程，
 * 且该协程为当前线程的主协程，其他协程都通过这个协程来调度，也就是说，其他协程
 * 结束时,都要切回到主协程，由主协程重新选择新的协程进行resume
 * @attention 线程如果要创建协程，那么应该首先执行一下Fiber::GetThis()操作，以初始化主函数协程
 */
sylar::Fiber::ptr Fiber::GetThis() {
  if (t_fiber) {
    return t_fiber->shared_from_this();
  }
  Fiber::ptr main_fiber(new Fiber);
  // Fiber::ptr main_fiber = std::make_shared<Fiber>();  // fiber构造函数为私有
  SYLAR_ASSERT(t_fiber == main_fiber.get());
  t_threadFiber = main_fiber;
  return t_fiber->shared_from_this();
}

// 协程切换到后台，并且设置为Ready状态
// 半路yield的协程没有执行完时，这种yield调度器会再次将协程加入任务队列并等待调度，
void Fiber::YieldToReady() {
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur->m_state == EXEC);
  cur->m_state = READY;
  cur->swapOut();
}

// 协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur->m_state == EXEC);
  // cur->m_state = HOLD;
  cur->swapOut();
}
// 总协程数
u_int64_t Fiber::TotalFibers() { return s_fiber_count; }

// 线程的主协程不会进入到MainFunc中
void Fiber::MainFunc() {
  Fiber::ptr cur = GetThis();  // GetThis()的shared_from_this()方法让引用计数加1
  SYLAR_ASSERT(cur);
  try {
    cur->m_cb();  // 这里真正执行协程的入口函数
    cur->m_cb = nullptr;
    cur->m_state = TERM;
  } catch (std::exception &ex) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber_id=" << cur->getId()
                              << std::endl
                              << sylar::BacktraceToString();
  } catch (...) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                              << " fiber_id=" << cur->getId() << std::endl
                              << sylar::BacktraceToString();
  }

  auto raw_ptr = cur.get();  // 手动让t_fiber的引用计数减1
  cur.reset();
  raw_ptr->swapOut();  // 协程结束时自动swapOut，以回到主协程(调度协程)

  SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() {
  Fiber::ptr cur = GetThis();  // GetThis()的shared_from_this()方法让引用计数加1
  SYLAR_ASSERT(cur);
  try {
    cur->m_cb();  // 这里真正执行协程的入口函数
    cur->m_cb = nullptr;
    cur->m_state = TERM;
  } catch (std::exception &ex) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber_id=" << cur->getId()
                              << std::endl
                              << sylar::BacktraceToString();
  } catch (...) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                              << " fiber_id=" << cur->getId() << std::endl
                              << sylar::BacktraceToString();
  }

  auto raw_ptr = cur.get();  // 手动让t_fiber的引用计数减1
  cur.reset();
  // 和MianFunc的区别
  raw_ptr->back();  // 协程结束时自动swapOut，这里回到主线程的主协程

  SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}
}  // namespace sylar
