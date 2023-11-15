#include "sylar/timer.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include "sylar/iomanager.h"
#include "sylar/util.h"

namespace sylar {
bool Timer::Comparator::operator()(const Timer::ptr &lhs,
                                   const Timer::ptr &rhs) const {
  if (!lhs && !rhs) {
    return false;
  }
  if (!lhs) {
    return true;
  }
  if (!rhs) {
    return false;
  }
  if (lhs->m_next < rhs->m_next) {
    return true;
  }
  if (lhs->m_next > rhs->m_next) {
    return false;
  }
  return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool resurring,
             TimerManager *manager)
    : m_recurring(resurring), m_ms(ms), m_cb(cb), m_manager(manager) {
  m_next = sylar::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) : m_next(next) {}

bool Timer::cancel() {
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (m_cb) {
    m_cb = nullptr;
    auto it = m_manager->m_timers.find(shared_from_this());
    m_manager->m_timers.erase(it);
    return true;
  }
  return false;
}

// 数据5秒内没收到就会触发，3秒内收到，就要重新设置定时器
bool Timer::refresh() {
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (!m_cb) {
    return false;
  }

  auto it = m_manager->m_timers.find(shared_from_this());
  if (it == m_manager->m_timers.end()) {
    return false;
  }

  // 先移除掉，重置时间，然后塞回去-- ？？why
  // -- set不提供直接存取元素的任何操作函数
  // operator
  // 是基于时间来做的，如果直接改时间，会影响他的比较位置，整个数据结构都会有问题,排序准则
  m_manager->m_timers.erase(it);
  m_next = sylar::GetCurrentMS() + m_ms;
  m_manager->m_timers.insert(shared_from_this());
  return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
  /// 不从现在开始计时
  if (ms == m_ms && !from_now) {
    return true;
  }
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (!m_cb) {
    return false;
  }

  auto it = m_manager->m_timers.find(shared_from_this());
  if (it == m_manager->m_timers.end()) {
    return false;
  }

  m_manager->m_timers.erase(it);
  uint64_t start = 0;
  // 是否从当前时间开始计算
  if (from_now) {
    /// 改为现在开始
    start = sylar::GetCurrentMS();
  } else {
    /// 获取起始时间
    start = m_next - m_ms;
  }
  m_ms = ms;
  m_next = start + m_ms;
  m_manager->addTimer(shared_from_this(), lock);

  return true;
}

TimerManager::TimerManager() { m_previouseTime = sylar::GetCurrentMS(); }
TimerManager::~TimerManager() {}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                                  bool recurring) {
  Timer::ptr timer(new Timer(ms, cb, recurring, this));
  // Timer::ptr timer = std::make_shared<Timer>(ms, cb, recurring, this);  //
  // timer 私有构造，无法使用make_shared

  RWMutexType::WriteLock lock(m_mutex);
  addTimer(timer, lock);
  return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
  std::shared_ptr<void> tmp = weak_cond.lock();
  if (tmp) {
    cb();
  }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond,
                                           bool recurring) {
  return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
  RWMutexType::ReadLock lock(m_mutex);
  m_tickled = false;

  if (m_timers.empty()) {
    // Value = 18446744073709551615 (0xffffffffffffffff)
    return ~0ull;
  }

  const Timer::ptr &next = *m_timers.begin();
  uint64_t now_ms = sylar::GetCurrentMS();
  /// 已经超时，需要执行了，，直接返回 0
  if (now_ms >= next->m_next) {
    return 0;
  } else {
    /// 返回到m_next时间，需要等待的时间
    return next->m_next - now_ms;
  }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
  uint64_t now_ms = sylar::GetCurrentMS();
  std::vector<Timer::ptr> expired;
  {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_timers.empty()) {
      return;
    }
  }
  RWMutexType::WriteLock lock(m_mutex);
  /// 检查是否时间调后了，会导致任务无法按时执行。
  bool rollover = detextClockRollover(now_ms);
  /// 没有重调时间，且超时任务还没到达
  if (!rollover && ((*m_timers.begin())->m_next > now_ms)) {
    return;
  }
  /// 参考时间
  Timer::ptr now_timer(new Timer(now_ms));
  // lower_bound(
  // begin,end,num)：从数组的begin位置到end-1位置二分查找第一个大于或等于num的数字，找到返回该数字的地址，不存在则返回end。
  // 找到所有需要执行的定时器
  auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
  // 处理 (*it)->m_next == now 的情况，
  while (it != m_timers.end() && (*it)->m_next == now_ms) {
    ++it;
  }
  expired.insert(expired.begin(), m_timers.begin(), it);
  m_timers.erase(m_timers.begin(), it);
  /// 预分配内存
  cbs.reserve(expired.size());

  for (auto &timer : expired) {
    cbs.push_back(timer->m_cb);
    if (timer->m_recurring) {
      timer->m_next = now_ms + timer->m_ms;
      m_timers.insert(timer);
    } else {
      /// 应该就析构了，不需要清空。
      timer->m_cb = nullptr;
    }
  }
}

// 当TimerManager检测到新添加的定时器的超时时间比当前最小的定时器还要小时，TimerManager通过这个方法来通知IOManager立刻更新当前的epoll_wait超时，
// 否则新添加的定时器的执行时间将不准确。实际实现时，只需要在onTimerInsertedAtFront()
// 方法内执行一次tickle就行了，tickle之后，epoll_wait会立即退出，并重新从TimerManager中获取最近的超时时间，这时拿到的超时时间就是新添加的最小定时器的超时时间了。
void TimerManager::addTimer(Timer::ptr timer, RWMutexType::WriteLock &lock) {
  auto it = m_timers.insert(timer).first;
  bool at_front = (it == m_timers.begin()) && !m_tickled;
  if (at_front) {
    m_tickled = true;
  }
  lock.unlock();
  // 不需要来回修改，只修改一次
  /// 是否在最靠前
  if (at_front) {
    onTimerInsertedAtFront();
  }
}

// 传进来的时间比他小，并且比他一个小时之前按还小，有问题
bool TimerManager::detextClockRollover(uint64_t now_ms) {
  bool rollover = false;
  // 现在的时间比上次执行的时间还小，并且比上次执行的时间的一小时之前还小，有问题，会出现吗？？？
  if (now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000)) {
    rollover = true;
  }
  // 重置 上次执行的时间
  m_previouseTime = now_ms;
  return rollover;
}

bool TimerManager::hasTimer() {
  RWMutexType::ReadLock lock(m_mutex);
  return !m_timers.empty();
}
}  // namespace sylar