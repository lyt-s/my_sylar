#ifndef SYLAR_SCHEDULE_H_
#define SYLAR_SCHEDULE_H_

#include <atomic>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "fiber.h"
#include "thread.h"
namespace sylar {
// base
class Scheduler {
 public:
  typedef std::shared_ptr<Scheduler> ptr;
  typedef Mutex MutexType;

  Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");
  ~Scheduler();

  const std::string &getName() const { return m_name; }

  static Scheduler *GetThis();
  static Fiber *GetMainFiber();

  void start();
  void stop();

  template <class FiberOrCb>
  void schedule(FiberOrCb fc, int thread = -1) {
    bool need_tickle = false;
    {
      MutexType::Lock lock(m_mutex);
      need_tickle = scheduleNoLock(fc, thread);
    }
    if (need_tickle) {
      tickle();
    }
  }

  template <class InputIterator>
  void schedule(InputIterator begin, InputIterator end) {
    bool need_tickle = false;
    {
      MutexType::Lock lock(m_mutex);
      while (begin != end) {
        need_tickle = scheduleNoLock(&*begin) || need_tickle;
      }
    }
    if (need_tickle) {
      tickle();
    }
  }

 protected:
  void setThis();
  virtual void tickle();
  void run();
  virtual bool stopping();

  // 没有任务时
  virtual void idle();

 private:
  // 模板搭配函数重载--todo
  template <class FiberOrCb>
  bool scheduleNoLock(FiberOrCb fc, int thread) {
    bool need_tickle = m_fibers.empty();
    FiberAndThread ft(fc, thread);
    if (ft.fiber || ft.cb) {
      m_fibers.push_back(ft);
    }
    return need_tickle;
  }

 private:
  struct FiberAndThread {
    Fiber::ptr fiber;
    std::function<void()> cb;
    int thread;

    FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}

    FiberAndThread(Fiber::ptr *f, int thr) : thread(thr) { fiber.swap(*f); }

    FiberAndThread(std::function<void()> *f, int thr) : thread(thr) { cb.swap(*f); }

    FiberAndThread() : thread(-1) {}

    void reset() {
      fiber = nullptr;
      cb = nullptr;
      thread = -1;
    }
  };

 private:
  MutexType m_mutex;
  std::vector<Thread::ptr> m_threads;
  Fiber::ptr m_rootFiber;
  std::list<FiberAndThread> m_fibers;
  std::string m_name;

  // 当前执行线程
 protected:
  std::vector<int> m_threadIds;
  size_t m_threadCount = 0;
  std::atomic<size_t> m_activeThreadCount = {0};
  std::atomic<size_t> m_idleThreadCount = {0};
  bool m_stopping = true;
  bool m_autoStop = false;
  int m_rootThread = 0;
  // 空闲线程
};
}  // namespace sylar
#endif  // SYLAR_SCHEDULE_H_