#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/thread.h"
#include "sylar/util.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int count = 0;
sylar::RWMutex s_mutex;

void fun1() {
  SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                           << "this.name: " << sylar::Thread::GetThis()->getName()
                           << "id: " << sylar::GetThreadId()
                           << "this.id: " << sylar::Thread::GetThis()->getId();

  for (uint32_t i = 0; i < 1000000; ++i) {
    // sylar::RWMutex::WriteLock lock(s_mutex);
    // sylar::RWMutex::ReadLock lock(s_mutex);
    ++count;
  }
}

void fun2() {
  while (true) {
    SYLAR_LOG_INFO(g_logger) << "XXXXXXXXXXXXXXXXXXXXXXXX";
  }
}

void fun3() {
  while (true) {
    SYLAR_LOG_INFO(g_logger) << "===============================";
  }
}

int main() {
  SYLAR_LOG_INFO(g_logger) << "thread test begin";
  YAML::Node root = YAML::LoadFile("../template/bin/conf/log2.yml");
  sylar::Config::LoadFromYaml(root);
  std::vector<sylar::Thread::ptr> thrs;

  for (int i = 0; i < 2; ++i) {
    sylar::Thread::ptr thr(new sylar::Thread(&fun2, "name_" + std::to_string(i * 2)));
    sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));

    thrs.push_back(thr);
    thrs.push_back(thr2);
  }
  for (size_t i = 0; i < thrs.size(); ++i) {
    thrs[i]->join();
  }
  SYLAR_LOG_INFO(g_logger) << "thread test end";
  SYLAR_LOG_INFO(g_logger) << "count=" << count;

  return 0;
}