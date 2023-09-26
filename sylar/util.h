#ifndef SYLAR_UTIL_H_
#define SYLAR_UTIL_H_
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
namespace sylar {

pid_t GetThreadId();

uint32_t GetFiberId();
}  // namespace sylar
#endif