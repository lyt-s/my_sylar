
#include <unistd.h>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include "env.h"
#include "hook.h"

// 利用 /proc/pid/cmdline, 和全局变量构造函数，实现在进入main函数前解析函数
struct A {
  A() {
    std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline",
                      std::ios::binary);
    std::string content;
    content.resize(4096);

    ifs.read(&content[0], content.size());
    content.resize(ifs.gcount());

    for (size_t i = 0; i < content.size(); ++i) {
      std::cout << i << " - " << content[i] << " - " << (int)content[i]
                << std::endl;
    }
    std::cout << content << std::endl;
  }
};

A a;

// 1. 读写环境变量
// 2. 获取程序的绝对路径，基于绝对路径设置cwd
// 3. 利用 /proc/pid/cmdline, 和全局变量构造函数，实现在进入main函数前解析函数

int main(int argc, char **argv) {
  std::cout << "argc=" << argc << std::endl;
  sylar::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
  sylar::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
  sylar::EnvMgr::GetInstance()->addHelp("p", "print help");

  if (!sylar::EnvMgr::GetInstance()->init(argc, argv)) {
    sylar::EnvMgr::GetInstance()->printHelp();
    return 0;
  }

  std::cout << "exe=" << sylar::EnvMgr::GetInstance()->getExe() << std::endl;
  std::cout << "cwd=" << sylar::EnvMgr::GetInstance()->getCwd() << std::endl;

  std::cout << "path=" << sylar::EnvMgr::GetInstance()->getEnv("PATH", "XXX")
            << std::endl;
  std::cout << "test=" << sylar::EnvMgr::GetInstance()->getEnv("TEST", "XX")
            << std::endl;
  std::cout << "SET test=" << sylar::EnvMgr::GetInstance()->setEnv("TEST", "yy")
            << std::endl;

  std::cout << "test=" << sylar::EnvMgr::GetInstance()->getEnv("TEST", "xx")
            << std::endl;

  if (sylar::EnvMgr::GetInstance()->has("p")) {
    sylar::EnvMgr::GetInstance()->printHelp();
  }
  return 0;
}