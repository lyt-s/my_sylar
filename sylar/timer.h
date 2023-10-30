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

/**
 * @brief 定时器
 */
class Timer : public std::enable_shared_from_this<Timer> {
  friend class TimerManager;

 public:
  typedef std::shared_ptr<Timer> ptr;
  /**
   * @brief 取消定时器
   *  从TimerManager的m_timers中找到对应的Timer::ptr ,erase
   * @return true
   * @return false
   */
  bool cancel();

  /**
   * @brief 刷新设置定时器的执行时间
   *
   * @return true
   * @return false
   */
  bool refresh();

  /**
   * @brief 重置定时器时间
   *
   * @param ms 定时器执行间隔时间(毫秒)
   * @param from_now  是否从当前时间开始计算
   * @return true
   * @return false
   */
  bool reset(uint64_t ms, bool from_now);

 private:
  /**
   * @brief 构造函数
   * @param[in] ms 定时器执行间隔时间
   * @param[in] cb 回调函数
   * @param[in] recurring 是否循环
   * @param[in] manager 定时器管理器
   */
  Timer(uint64_t ms, std::function<void()> cb, bool resurring,
        TimerManager *manager);
  Timer(uint64_t next);

 private:
  bool m_recurring = false;  // 是否循环定时器
  uint64_t m_ms = 0;         // 执行周期
  uint64_t m_next = 0;       // 精确的执行时间
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

  /**
   * @brief 添加定时器
   * @param[in] ms 定时器执行间隔时间
   * @param[in] cb 定时器回调函数
   * @param[in] recurring 是否循环定时器
   */
  Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                      bool recurring = false);
  /**
   * @brief 添加条件定时器
   * @param[in] ms 定时器执行间隔时间
   * @param[in] cb 定时器回调函数
   * @param[in] weak_cond 条件
   * @param[in] recurring 是否循环
   */
  Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                               std::weak_ptr<void> weak_cond,
                               bool recurring = false);
  /**
   * @brief 到最近一个定时器执行的时间间隔(毫秒)
   *
   * @return uint64_t
   */
  uint64_t getNextTimer();

  /**
   * @brief 获取需要执行的定时器的回调函数列表
   *
   * @param[out] cbs 回调函数数组
   */
  void listExpiredCb(std::vector<std::function<void()>> &cbs);
  /**
   * @brief 是否有定时器
   */
  bool hasTimer();

 protected:
  /**
   * @brief 当有新的定时器插入到定时器的首部,执行该函数,IoManager来实现
   */
  virtual void onTimerInsertedAtFront() = 0;
  /**
   * @brief 将定时器添加到管理器中
   */
  void addTimer(Timer::ptr val, RWMutexType::WriteLock &lock);

 private:
  /**
   * @brief 检测服务器时间是否被调后了
   */
  bool detextClockRollover(uint64_t now_ms);

 private:
  RWMutexType m_mutex;
  /// 这个参数不代表映射对象，而是传递一个比较结构体
  /// 定时器集合
  std::set<Timer::ptr, Timer::Comparator> m_timers;
  /// 是否触发onTimerInsertedAtFront
  bool m_tickled = false;
  /// 上次执行时间
  uint64_t m_previousTime = 0;
};

}  // namespace sylar

#endif  // SYLAR_TIMER_H_