#ifndef SYLAR_ENV_H_
#define SYLAR_ENV_H_

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "singleton.h"
#include "thread.h"
namespace sylar {
class Env {
 public:
  typedef RWMutex RWMutexType;

  // argc 是 你执行程序命令的路径，而不是执行文件的路径， 现在改为后者
  bool init(int argc, char **argv);

  void add(const std::string &key, const std::string &val);
  bool has(const std::string &key);
  void del(const std::string &key);
  std::string get(const std::string &key, const std::string &default_val = "");

  void addHelp(const std::string &key, const std::string &desc);
  void removeHelp(const std::string &key);
  void printHelp();

  const std::string &getCwd() const { return m_cwd; }
  const std::string &getExe() const { return m_exe; }

  bool setEnv(const std::string &key, const std::string &val);
  std::string getEnv(const std::string &key,
                     const std::string &default_val = "");

  std::string getAbsolutePath(const std::string &path) const;

 private:
  RWMutexType m_mutex;
  // std::string m_absolute_path;
  std::map<std::string, std::string> m_args;
  std::vector<std::pair<std::string, std::string>> m_helps;
  std::string m_program;
  std::string m_cwd;
  std::string m_exe;
};

typedef sylar::Singleton<Env> EnvMgr;
}  // namespace sylar

#endif  // SYLAR_ENV_H_