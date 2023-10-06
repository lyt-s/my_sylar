#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <pthread.h>
#include <semaphore.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace sylar {

// 信号量 主要用在线程构造函数，在确保出线程构造函数之前，要启动的线程已经启动了
class Semaphore {
 public:
  Semaphore(uint32_t count = 0);
  ~Semaphore();

  void wait();
  void notify();

 private:
  Semaphore(const Semaphore &) = delete;
  Semaphore(const Semaphore &&) = delete;
  Semaphore &operator=(const Semaphore &) = delete;

 private:
  sem_t m_semaphore;
};

template <class T>
struct ScopeLockImpl {
 public:
  ScopeLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.lock();
    m_locked = true;
  }
  ~ScopeLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.lock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

 private:
  T &m_mutex;
  bool m_locked;
};

template <class T>
struct ReadScopeLockImpl {
 public:
  ReadScopeLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.rdlock();
    m_locked = true;
  }
  ~ReadScopeLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.rdlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

 private:
  T &m_mutex;
  bool m_locked;
};

template <class T>
struct WriteScopeLockImpl {
 public:
  WriteScopeLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.wrlock();
    m_locked = true;
  }
  ~WriteScopeLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.wrlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

 private:
  T &m_mutex;
  bool m_locked;
};

class Mutex {
 public:
  typedef ScopeLockImpl<Mutex> Lock;
  Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
  ~Mutex() { pthread_mutex_destroy(&m_mutex); }

  void lock() { pthread_mutex_lock(&m_mutex); }

  void unlock() { pthread_mutex_unlock(&m_mutex); }

 private:
  pthread_mutex_t m_mutex;
};
// 互斥量
class RWMutex {
 public:
  typedef ReadScopeLockImpl<RWMutex> ReadLock;
  typedef WriteScopeLockImpl<RWMutex> WriteLock;

  RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }
  ~RWMutex() { pthread_rwlock_destroy(&m_lock); }

  void rdlock() { pthread_rwlock_rdlock(&m_lock); }

  void wrlock() { pthread_rwlock_wrlock(&m_lock); }

  void unlock() { pthread_rwlock_unlock(&m_lock); }

 private:
  pthread_rwlock_t m_lock;
};

class Spinlock {
 public:
  typedef ScopeLockImpl<Spinlock> Lock;
  Spinlock() { pthread_spin_init(&m_mutex, 0); }

  ~Spinlock() { pthread_spin_destroy(&m_mutex); }

  void lock() { pthread_spin_lock(&m_mutex); }

  void unlock() { pthread_spin_unlock(&m_mutex); }

 private:
  pthread_spinlock_t m_mutex;
};

// todo
class CASLock {
 public:
  typedef ScopeLockImpl<CASLock> Lock;
  CASLock() { m_mutex.clear(); }
  ~CASLock() {}
  void lock() {
    while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire)) {
    };
  }
  void unlock() { std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release); }

 private:
  // todo 什么意思
  volatile std::atomic_flag m_mutex;
};

class Thread {
 public:
  using ptr = std::shared_ptr<Thread>;
  Thread(std::function<void()> cb, const std::string &name);
  ~Thread();

  pid_t getId() const { return m_id; }
  const std::string &getName() const { return m_name; }

  void join();

  static Thread *GetThis();
  static const std::string &GetName();
  static void SetName(const std::string &name);

 private:
  Thread(const Thread &) = delete;
  Thread(const Thread &&) = delete;
  Thread &operator=(const Thread &) = delete;
  static void *run(void *arg);

 private:
  pid_t m_id = -1;
  pthread_t m_thread = 0;
  std::function<void()> m_cb;
  std::string m_name;
  Semaphore m_semaphore;
};
}  // namespace sylar
#endif