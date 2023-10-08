#include "sylar/log.h"

#include "sylar/fiber.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber() {
  SYLAR_LOG_INFO(g_logger) << "run in fiber begin";
  //   sylar::Fiber::GetThis()->swapOut();
  sylar::Fiber::YieldToHold();
  SYLAR_LOG_INFO(g_logger) << "run in fiber end";
  sylar::Fiber::YieldToHold();
}

int main() {
  // segmentation fault
  sylar::Fiber::GetThis();

  SYLAR_LOG_INFO(g_logger) << "main begin";

  sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
  fiber->swapIn();
  SYLAR_LOG_INFO(g_logger) << "main after swapIn";
  fiber->swapIn();
  SYLAR_LOG_INFO(g_logger) << "main after end";
  fiber->swapIn();
  return 0;
}