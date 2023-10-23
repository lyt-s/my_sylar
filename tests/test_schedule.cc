#include <unistd.h>
#include <iostream>
#include "sylar/fiber.h"
#include "sylar/log.h"
#include "sylar/schedule.h"
#include "util.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber() {
  static int s_count = 5;
  SYLAR_LOG_INFO(g_logger) << "test in fiber s_count =" << s_count;

  // sleep(1);  // bug 在这里  死锁了
  if (--s_count >= 0) {
    // 在指定线程执行
    sylar::Scheduler::GetThis()->schedule(&test_fiber);
  }
}
int main() {
  SYLAR_LOG_INFO(g_logger) << "main";
  // // bug--已解决
  sylar::Scheduler sc(1, true, "test");
  // sylar::Scheduler sc(2, true, "test");

  sc.start();
  // sleep(2);
  SYLAR_LOG_INFO(g_logger) << "schedule";
  // 添加任务

  sc.schedule(&test_fiber);
  sc.stop();
  SYLAR_LOG_INFO(g_logger) << "over";
  return 0;
}