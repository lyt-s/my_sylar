#ifndef SYLAR_UTIL_H_
#define SYLAR_UTIL_H_
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
#include <string>
#include <vector>
namespace sylar {

pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);
std::string BacktraceToString(int size, int skip = 2, const std::string &prefix = "");

}  // namespace sylar
#endif