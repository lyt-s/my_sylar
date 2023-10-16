#ifndef SYLAR_IOMANAGER_H_
#define SYLAR_IOMANAGER_H_

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "config.h"
#include "fiber.h"
#include "schedule.h"
#include "thread.h"
namespace sylar {
class IOManger : public Scheduler {
 public:
  typedef std::shared_ptr<IOManger> ptr;
  typedef RWMutex RWMutexType;

  enum Event {
    NONE = 0x0,
    READ = 0x1,
    WRITE = 0x2,

  };

 private:
  struct FdContext {
    typedef Mutex MutexType;
    struct EventContext {
      Scheduler *scheduler;      // 待执行的schedule
      Fiber::ptr fiber;          //事件的协程
      std::function<void()> cb;  // 时间的回调函数
    };

    EventContext &getContext(Event event);
    void resetContext(EventContext &ctx);
    void triggerEvent(Event event);

    EventContext read;      // 读事件
    EventContext write;     // 写事件
    int fd;                 // 事件关联的句柄
    Event m_events = NONE;  // 已经注册的事件
    MutexType m_mutex;
  };

 public:
  IOManger(size_t threads = 1, bool use_caller = true, const std::string &name = " ");
  ~IOManger();

  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
  bool delEvent(int fd, Event event);
  bool cancelEvent(int fd, Event event);

  bool cancelAll(int fd);

  static IOManger *GetThis();

 protected:
  void tickle() override;

  bool stopping() override;

  void idle() override;

  void contextResize(size_t size);

 private:
  int m_epfd = 0;
  int m_tickleFds[2];

  std::atomic<size_t> m_pendingEventCount = {0};
  RWMutexType m_mutex;
  std::vector<FdContext *> m_fdContexts;
};
}  // namespace sylar
#endif  // SYLAR_IOMANAGER_H_