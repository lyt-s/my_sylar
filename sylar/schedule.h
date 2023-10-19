#ifndef SYLAR_SCHEDULE_H_
#define SYLAR_SCHEDULE_H_

#include <atomic>
#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <vector>
#include "fiber.h"
#include "thread.h"

namespace sylar {

/**
 * 1. 线程池，分配一组线程
 * 2. 协程调度器，将协程指定到相应的线程上去执行
 */

/**
 * @brief 协程调度器
 * @details 封装的是N-M的协程调度器
 *          内部有一个线程池,支持协程在线程池里面切换
 */
class Scheduler {
 public:
  typedef std::shared_ptr<Scheduler> ptr;
  typedef Mutex MutexType;

  /**
   * @brief 构造函数
   * @param[in] threads 线程数量
   * @param[in] use_caller 是否使用当前调用线程，true的话就会纳入到调度中
   * @param[in] name 协程调度器名称，线程池名称，
   */
  Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");

  /**
   * @brief 析构函数
   */
  virtual ~Scheduler();

  /**
   * @brief 返回协程调度器名称
   */
  const std::string &getName() const { return m_name; }

  /**
   * @brief 返回当前协程调度器
   */
  static Scheduler *GetThis();

  /**
   * @brief 返回当前协程调度器的调度协程，---主协程
   */
  static Fiber *GetMainFiber();

  /**
   * @brief 启动协程调度器，线程池
   */
  void start();

  /**
   * @brief 停止协程调度器，线程池
   */
  void stop();

  /**
   * @brief 调度协程
   * @param[in] fc 协程或函数
   * @param[in] thread 协程执行的线程id,-1标识任意线程
   */
  template <class FiberOrCb>
  void schedule(FiberOrCb fc, int thread = -1) {
    bool need_tickle = false;
    {
      MutexType::Lock lock(m_mutex);
      need_tickle = scheduleNoLock(fc, thread);
    }

    // 待执行队列为空时，然后添加了一个带执行的任务， 则执行下面语句，具体细节查看scheduleNoLock
    if (need_tickle) {
      tickle();
    }
  }

  /**
   * @brief 批量调度协程
   * @param[in] begin 协程数组的开始
   * @param[in] end 协程数组的结束
   */
  template <class InputIterator>
  void schedule(InputIterator begin, InputIterator end) {
    bool need_tickle = false;
    {
      MutexType::Lock lock(m_mutex);
      while (begin != end) {
        // 参数为指针，取得是地址，会将里面的东西 swap掉。---这里怎么确认是callback还是fiber？？
        need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
        // 没有++begin  --》死循环
        ++begin;  // 注意迭代器问题
      }
    }
    if (need_tickle) {
      tickle();
    }
  }

  void switchTo(int thread = -1);
  std::ostream &dump(std::ostream &os);

 protected:
  /**
   * @brief 通知协程调度器有任务了
   */

  virtual void tickle();
  /**
   * @brief 协程调度函数
   */
  void run();

  /**
   * @brief 返回是否可以停止
   */
  virtual bool stopping();

  /**
   * @brief 协程无任务可调度时执行idle协程
   */
  virtual void idle();

  /**
   * @brief 设置当前的协程调度器
   */
  void setThis();

  /**
   * @brief 是否有空闲线程
   */
  bool hasIdleThreads() { return m_idleThreadCount > 0; }

 private:
  /**
   * @brief 协程调度启动(无锁)
   */
  template <class FiberOrCb>
  bool scheduleNoLock(FiberOrCb fc, int thread) {
    bool need_tickle = m_fibers.empty();  // true = 空的待执行队列
    FiberAndThread ft(fc, thread);
    // 是协程或者function
    if (ft.fiber || ft.cb) {
      m_fibers.push_back(ft);
    }
    // 以前是空的，加上一个后不为空了，我就需要唤醒线程，有任务来了
    return need_tickle;
  }

 private:
  /**
   * @brief 协程/函数/线程组
   */
  struct FiberAndThread {
    /// 协程
    Fiber::ptr fiber;
    /// 协程执行函数
    std::function<void()> cb;
    /// 线程id，---需要指定协程在哪一个线程上执行
    int thread;

    /**
     * @brief 构造函数
     * @param[in] f 协程
     * @param[in] thr 线程id
     */
    FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}

    /**
     * @brief 构造函数
     * @param[in] f 协程指针
     * @param[in] thr 线程id
     * @post *f = nullptr---swap  todo
     */
    FiberAndThread(Fiber::ptr *f, int thr) : thread(thr) { fiber.swap(*f); }

    /**
     * @brief 构造函数
     * @param[in] f 协程执行函数
     * @param[in] thr 线程id
     */
    FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {}

    /**
     * @brief 构造函数
     * @param[in] f 协程执行函数指针
     * @param[in] thr 线程id
     * @post *f = nullptr
     */
    FiberAndThread(std::function<void()> *f, int thr) : thread(thr) { cb.swap(*f); }

    /**
     * @brief 无参构造函数
     * stl 中一定需要默认构造函数
     */
    FiberAndThread() : thread(-1) {}

    /**
     * @brief 重置数据
     */
    void reset() {
      fiber = nullptr;
      cb = nullptr;
      thread = -1;
    }
  };

 private:
  /// Mutex
  MutexType m_mutex;
  /// 线程池
  std::vector<Thread::ptr> m_threads;
  /// 待执行的协程队列
  std::list<FiberAndThread> m_fibers;
  /// use_caller为true时有效, 调度协程
  Fiber::ptr m_rootFiber;
  /// 协程调度器名称
  std::string m_name;

 protected:
  /// 协程下的线程id数组
  std::vector<int> m_threadIds;
  /// 线程数量
  size_t m_threadCount = 0;
  /// 工作线程数量
  std::atomic<size_t> m_activeThreadCount = {0};
  /// 空闲线程数量
  std::atomic<size_t> m_idleThreadCount = {0};
  /// 用来表示Scheduler::start()是否启动，默认表示未启动
  bool m_stopping = true;
  /// 是否自动停止
  // 这个标志会使得调度器在调度完所有任务后自动退出。
  bool m_autoStop = false;
  /// 主线程id(use_caller)
  int m_rootThread = 0;
};
}  // namespace sylar
#endif  // SYLAR_SCHEDULE_H_