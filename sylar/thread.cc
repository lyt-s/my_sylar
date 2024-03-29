#include "thread.h"
#include <asm-generic/errno-base.h>
#include <pthread.h>
#include <semaphore.h>
#include <cerrno>
#include <functional>
#include <stdexcept>
#include <string>
#include "log.h"
#include "util.h"

namespace sylar {

static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("syatem");

Semaphore::Semaphore(uint32_t count) {
  if (sem_init(&m_semaphore, 0, count)) {
    throw std::logic_error("sem_init error");
  }
}

Semaphore::~Semaphore() { sem_destroy(&m_semaphore); }
void Semaphore::wait() {
  if (sem_wait(&m_semaphore)) {
    throw std::logic_error("sem_wait error");
  }
}

void Semaphore::notify() {
  if (sem_post(&m_semaphore)) {
    throw std::logic_error("sem_post error");
  }
}

Thread *Thread::GetThis() { return t_thread; }
const std::string &Thread::GetName() { return t_thread_name; }
void Thread::SetName(const std::string &name) {
  if (t_thread) {
    t_thread->m_name = name;
  }
  t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
    : m_cb(cb), m_name(name) {
  if (name.empty()) {
    m_name = "UNKNOW";
  }
  int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
  if (rt) {
    SYLAR_LOG_ERROR(g_logger)
        << "pthread_create thread fail, rt=" << rt << " name= " << name;
    throw std::logic_error("pthread_create error");
  }
  // 有的时候，创建的线程会在线程的构造函数执行完成之后，还未执行，为了防止此情况发生。
  // 确保线程跑起来。// 阻塞去获取信号量，信号量的释放是在
  // run函数中，这样可以确保线程跑起来
  m_semaphore.wait();
}
Thread::~Thread() {
  if (m_thread) {
    // 析构里，detach
    pthread_detach(m_thread);
  }
}

void Thread::join() {
  if (m_thread) {
    int rt = pthread_join(m_thread, nullptr);
    if (rt) {
      SYLAR_LOG_ERROR(g_logger)
          << "pthread_join thread fail, rt=" << rt << " name= " << m_name;
      throw std::logic_error("pthread_join error");
    }
    m_thread = 0;
  }
}

void *sylar::Thread::run(void *arg) {
  Thread *thread = (Thread *)arg;
  t_thread = thread;
  t_thread_name = thread->m_name;
  thread->m_id = sylar::GetThreadId();
  // 给线程命名
  pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
  std::function<void()> cb;
  // 防止智能指针的引用，一直不会被释放的问题，swap少一个引用
  cb.swap(thread->m_cb);

  // 不能这样用
  // m_semaphore.notify();
  thread->m_semaphore.notify();
  cb();
  return 0;
}

const std::string &getName() { return t_thread_name; }

void join();

}  // namespace sylar