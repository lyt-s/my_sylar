#include <cassert>
#include "sylar/log.h"
#include "sylar/macro.h"
#include "sylar/util.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

// todo
void test_assert() {
  SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
  // SYLAR_ASSERT(false);
  SYLAR_ASSERT2(0 == 1, "abcdef xx");
}

int main() {
  test_assert();
  return 0;
}