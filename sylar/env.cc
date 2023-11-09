#include "env.h"
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <utility>
#include "log.h"
#include "thread.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 解析输入参数
bool Env::init(int argc, char **argv) {
  char link[1024]{0};
  char path[1024]{0};
  //放到link内
  sprintf(link, "/proc/%d/exe", getpid());
  __attribute__((unused)) int result = readlink(link, path, sizeof(path));
  // path/xxx/exe
  m_exe = path;

  auto pos = m_exe.find_last_of("/");
  m_cwd = m_exe.substr(0, pos) + "/";

  m_program = argv[0];
  // -config /path/to/config  -file xxxx
  const char *now_key = nullptr;
  // argc 中默认 0号位 ，为执行文件自己
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (strlen(argv[i]) > 1) {
        if (now_key) {
          add(now_key, "");
        }
        now_key = argv[i] + 1;
      } else {
        SYLAR_LOG_ERROR(g_logger)
            << "invalid arg idx=" << i << " val=" << argv[i];
        return false;
      }
    } else {
      if (now_key) {
        add(now_key, argv[i]);
        now_key = nullptr;
      } else {
        SYLAR_LOG_ERROR(g_logger)
            << "invalid arg idx=" << i << " val=" << argv[i];
        return false;
      }
    }
  }
  if (now_key) {
    add(now_key, "");
  }
  return true;
}

void Env::add(const std::string &key, const std::string &val) {
  RWMutexType::WriteLock lock(m_mutex);
  m_args[key] = val;
}
bool Env::has(const std::string &key) {
  RWMutexType::ReadLock lock(m_mutex);
  auto it = m_args.find(key);
  return it != m_args.end();
}

void Env::del(const std::string &key) {
  RWMutexType::WriteLock lock(m_mutex);
  m_args.erase(key);
}

std::string Env::get(const std::string &key, const std::string &default_val) {
  RWMutexType::ReadLock lock(m_mutex);
  auto it = m_args.find(key);
  return it != m_args.end() ? it->second : default_val;
}

void Env::addHelp(const std::string &key, const std::string &desc) {
  removeHelp(key);
  RWMutexType::WriteLock lock(m_mutex);
  m_helps.emplace_back(std::make_pair(key, desc));
}
void Env::removeHelp(const std::string &key) {
  RWMutexType::WriteLock lock(m_mutex);
  for (auto it = m_helps.begin(); it != m_helps.end();) {
    if (it->first == key) {
      // 返回的是，删除元素后面的迭代器
      it = m_helps.erase(it);
    } else {
      ++it;
    }
  }
}
void Env::printHelp() {
  RWMutexType::ReadLock lock(m_mutex);

  std::cout << "Uage: " << m_program << " [options]" << std::endl;
  for (auto &i : m_helps) {
    // todo
    std::cout << std::setw(5) << "-" << i.first << " : " << i.second
              << std::endl;
  }
}

bool Env::setEnv(const std::string &key, const std::string &val) {
  return !setenv(key.c_str(), val.c_str(), 1);
}
std::string Env::getEnv(const std::string &key,
                        const std::string &default_val) {
  // todo
  const char *v = getenv(key.c_str());
  if (v == nullptr) {
    return default_val;
  }
  return v;
}
}  // namespace sylar