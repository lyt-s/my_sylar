#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <string.h>
// 返回函数栈信息
#include <assert.h>
#include <execinfo.h>
#include "log.h"
#include "util.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define SYLAR_LICKLY(x) __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define SYLAR_UNLICKLY(x) __builtin_expect(!!(x), 0)
#else
#define SYLAR_LICKLY(x) (x)
#define SYLAR_UNLICKLY(x) (x)
#endif
// man assert

#define SYLAR_ASSERT(x)                                                            \
  if (SYLAR_UNLICKLY(!(x))) {                                                      \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n"      \
                                      << sylar::BacktraceToString(100, 2, "    "); \
    assert(x);                                                                     \
  }

#define SYLAR_ASSERT2(x, w)                                                        \
  if (SYLAR_UNLICKLY(!(x))) {                                                      \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x << "\n"                  \
                                      << w << "\nbacktrace:\n"                     \
                                      << sylar::BacktraceToString(100, 2, "    "); \
    assert(x);                                                                     \
  }

#endif  //__SYLAR_MACRO_H__