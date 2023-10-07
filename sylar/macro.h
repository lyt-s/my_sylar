#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <string.h>
// 返回函数栈信息
#include <execinfo.h>
#include "log.h"
#include "util.h"

// man assert

#define SYLAR_ASSERT(x)                                                            \
  if (!(x)) {                                                                      \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n"      \
                                      << sylar::BacktraceToString(100, 2, "    "); \
    assert(x);                                                                     \
  }

#define SYLAR_ASSERT2(x, w)                                                        \
  if (!(x)) {                                                                      \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x << "\n"                  \
                                      << w << "\nbacktrace:\n"                     \
                                      << sylar::BacktraceToString(100, 2, "    "); \
    assert(x);                                                                     \
  }

#endif  //__SYLAR_MACRO_H__