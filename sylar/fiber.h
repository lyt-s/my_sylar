#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__
// man ucontext.h
#include <sys/types.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <cstddef>
#include <functional>
#include <memory>

#include "thread.h"

namespace sylar {
class Scheduler;

// 当成智能指针，成员方法shared_from_this-->获取当前类的智能指针， 继承了enable_shared_from_this。
// 不能在栈上创建对象，构造函数会智能指针，在构造函数中不能使用shared_from_this
class Fiber : public std::enable_shared_from_this<Fiber> {
 public:
  friend class Scheduler;
  typedef std::shared_ptr<Fiber> ptr;
  enum State { INIT, HOLD, EXEC, TERM, READY, EXCEPT };

 private:
  Fiber();

 public:
  Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
  ~Fiber();

  //重置协程函数，并重置状态
  void reset(std::function<void()> cb);
  // 切换到当前协程执行
  void swapIn();
  // 把当前协程切换到后台
  void swapOut();

  void call();
  void back();

  u_int64_t getId() const { return m_id; }

  State getState() { return m_state; }

 public:
  // 设置当前协程
  static void SetThis(Fiber *f);
  static sylar::Fiber::ptr GetThis();
  // 协程切换到后台，并且设置为Ready状态
  static void YieldToReady();
  // 协程切换到后台，并且设置为Hold状态
  static void YieldToHold();
  // 总协程数
  static u_int64_t TotalFibers();

  static void MainFunc();
  static void CallerMainFunc();
  static u_int64_t GetFiberId();

 private:
  u_int64_t m_id = 0;
  u_int32_t m_stacksize = 0;
  State m_state = INIT;

  ucontext_t m_ctx;
  void *m_stack = nullptr;

  std::function<void()> m_cb;
};
}  // namespace sylar

#endif  //__SYLAR_FIBER_H__