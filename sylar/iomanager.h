#ifndef SYLAR_IOMANAGER_H_
#define SYLAR_IOMANAGER_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "config.h"
#include "fiber.h"
#include "schedule.h"
#include "thread.h"
#include "timer.h"

namespace sylar {
class IOManager final : public Scheduler, public TimerManager {
 public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef RWMutex RWMutexType;

  /**
   * @brief IO事件
   */
  enum Event {
    /// 无事件
    NONE = 0x0,
    /// 读事件(EPOLLIN)
    READ = 0x1,
    /// 写事件(EPOLLOUT)
    WRITE = 0x4,
  };

 private:
  /**
   * @brief Socket事件上下文类
   * @details 每个socket fd都对应一个FdContext，包括fd的值，fd上的事件，以及fd的读写事件上下文
   */
  struct FdContext {
    typedef Mutex MutexType;
    /**
     * @brief 事件上下文类
     */
    struct EventContext {
      /// 事件执行的调度器
      Scheduler *scheduler = nullptr;
      /// 事件协程
      Fiber::ptr fiber;
      /// 事件的回调函数
      std::function<void()> cb;
    };

    /**
     * @brief 获取事件上下文类
     * @param[in] event 事件类型
     * @return 返回对应事件的上下文
     */
    EventContext &getContext(Event event);

    /**
     * @brief 重置事件上下文
     * @param[in, out] ctx 待重置的事件上下文对象
     */
    void resetContext(EventContext &ctx);
    /**
     * @brief 触发事件
     * @details 根据事件类型调用对应上下文结构中的调度器去调度回调协程或回调函数
     * @param[in] event 事件类型
     */
    void triggerEvent(Event event);

    EventContext read;    // 读事件
    EventContext write;   // 写事件
    int fd = 0;           // 事件关联的句柄
    Event events = NONE;  // 已经注册的事件
    MutexType mutex;
  };

 public:
  /**
   * @brief 构造函数
   * @param[in] threads 线程数量
   * @param[in] use_caller 是否将调用线程包含进去
   * @param[in] name 调度器的名称
   */
  IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = " ");
  ~IOManager();

  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
  bool delEvent(int fd, Event event);
  bool cancelEvent(int fd, Event event);

  bool cancelAll(int fd);

  static IOManager *GetThis();

 protected:
  void tickle() override;
  // 协程调度是否应该终止
  bool stopping() override;
  void idle() override;
  void onTimerInsertedAtFront() override;

  void contextResize(size_t size);
  bool stopping(uint64_t &timeout);

 private:
  /// epoll 文件句柄
  int m_epfd = 0;
  /// pipe 文件句柄，fd[0]读端，fd[1]写端
  int m_tickleFds[2];
  /// 当前等待执行的IO事件数量
  std::atomic<size_t> m_pendingEventCount = {0};
  /// IOManager的Mutex
  RWMutexType m_mutex;
  /// socket事件上下文的容器
  std::vector<FdContext *> m_fdContexts;
};
}  // namespace sylar
#endif  // SYLAR_IOMANAGER_H_