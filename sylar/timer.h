#ifndef SYLAR_TIMER_H_
#define SYLAR_TIMER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <vector>
#include "thread.h"
namespace sylar {
class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
  friend class TimerManager;

 public:
  typedef std::shared_ptr<Timer> ptr;

  bool cancel();
  bool refresh();
  bool reset(uint64_t ms, bool from_now);

 private:
  Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);
  Timer(uint64_t next);

 private:
  bool m_recurring = false;  // 是否循环定时器
  uint64_t m_ms = 0;         //执行周期
  uint64_t m_next = 0;       //精确的执行时间
  std::function<void()> m_cb;
  TimerManager *m_manager = nullptr;

 private:
  struct Comparator {
    bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
  };
};

class TimerManager {
  friend class Timer;

 public:
  typedef RWMutex RWMutexType;

  TimerManager();
  virtual ~TimerManager();

  Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
  Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond,
                               bool recurring = false);
  uint64_t getNextTimer();
  void listExpiredCb(std::vector<std::function<void()>> &cbs);
  bool hasTimer();

 protected:
  virtual void onTimerInsertAtFront() = 0;
  void addTimer(Timer::ptr val, RWMutexType::WriteLock &lock);

 private:
  bool detectClockRollover(uint64_t now_ms);

 private:
  RWMutexType m_mutex;
  // 这个参数不代表映射对象，而是传递一个比较结构体 //todo
  std::set<Timer::ptr, Timer::Comparator> m_timers;
  bool m_tickled = false;
  uint64_t m_previouseTime = 0;
};

}  // namespace sylar

#endif  // SYLAR_TIMER_H_