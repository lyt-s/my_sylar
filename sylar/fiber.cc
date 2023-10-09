#include "fiber.h"

#include <sys/types.h>
#include <ucontext.h>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <memory>

#include "config.h"
#include "log.h"
#include "macro.h"
#include "thread.h"
namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<u_int64_t> s_fiber_id{0};
static std::atomic<u_int64_t> s_fiber_count{0};

static thread_local Fiber *t_fiber = nullptr;
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
  SetThis(this);

  if (getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
    ++s_fiber_count;
  }

  SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize) : m_id(++s_fiber_id), m_cb(cb) {
  ++s_fiber_count;
  m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

  m_stack = StackAllocator::Alloc(m_stacksize);
  if (getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;
  makecontext(&m_ctx, &Fiber::MainFunc, 0);

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

//重置协程函数，并重置状态
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
// 切换到当前协程执行
void Fiber::swapIn() {
  SetThis(this);
  SYLAR_ASSERT(m_state != EXEC);

  if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}
// 把当前协程切换到后台,main协程换出来
void Fiber::swapOut() {
  SetThis(t_threadFiber.get());

  if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

void Fiber::SetThis(Fiber *f) { t_fiber = f; }
sylar::Fiber::ptr Fiber::GetThis() {
  if (t_fiber) {
    return t_fiber->shared_from_this();
  }
  Fiber::ptr main_fiber(new Fiber);
  SYLAR_ASSERT(t_fiber == main_fiber.get());
  t_threadFiber = main_fiber;
  return t_fiber->shared_from_this();
}
// 协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
  Fiber::ptr cur = GetThis();
  cur->m_state = READY;
  cur->swapOut();
}
// 协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
  Fiber::ptr cur = GetThis();
  cur->m_state = HOLD;
  cur->swapOut();
}
// 总协程数
u_int64_t Fiber::TotalFibers() { return s_fiber_count; }

void Fiber::MainFunc() {
  // 问题在这里，智能指针引用加1
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur);
  try {
    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = TERM;
  } catch (std::exception &ex) {
    cur->m_state = EXCEPT;
    // ??
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
  } catch (...) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except: ";
  }

  // to do  为什么协程未释放指针，引用计数不小于1
  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->swapOut();

  SYLAR_ASSERT2(false, "never reach");
}
}  // namespace sylar
