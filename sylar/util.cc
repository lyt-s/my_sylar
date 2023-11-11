#include "util.h"

#include <bits/types/struct_timeval.h>
#include <bits/types/time_t.h>
#include <dirent.h>
#include <execinfo.h>
#include <ifaddrs.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>
#include "fiber.h"
#include "log.h"
namespace sylar {

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId() { return syscall(SYS_gettid); }
uint64_t GetFiberId() { return sylar::Fiber::GetFiberId(); }

void Backtrace(std::vector<std::string> &bt, int size, int skip) {
  //??
  void **array = (void **)malloc((sizeof(void *) * size));
  size_t s = ::backtrace(array, size);

  char **strings = backtrace_symbols(array, s);
  if (strings == NULL) {
    SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
    return;
  }

  for (size_t i = skip; i < s; ++i) {
    bt.push_back(strings[i]);
  }
  free(strings);
  free(array);
}
std::string BacktraceToString(int size, int skip, const std::string &prefix) {
  std::vector<std::string> bt;
  Backtrace(bt, size, skip);
  std::stringstream ss;
  for (size_t i = 0; i < bt.size(); ++i) {
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}

uint64_t GetCurrentMS() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

std::string Time2Str(time_t ts, const std::string &format) {
  struct tm tm;
  localtime_r(&ts, &tm);
  char buf[64];
  strftime(buf, sizeof(buf), format.c_str(), &tm);
  return buf;
}

void FSUtil::ListAllFile(std::vector<std::string> &files,
                         const std::string &path, const std::string &subfix) {
  // SYLAR_LOG_INFO(g_logger) << path;
  // c++17 文件系统todo
  // access 判断文件存不存在
  if (access(path.c_str(), 0) != 0) {
    return;
  }
  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    return;
  }
  struct dirent *dp = nullptr;
  while ((dp = readdir(dir)) != nullptr) {
    if (dp->d_type == DT_DIR) {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
        continue;
      }
      ListAllFile(files, path + "/" + dp->d_name, subfix);
    } else if (dp->d_type == DT_REG) {
      std::string filename(dp->d_name);
      if (subfix.empty()) {
        files.push_back(path + "/" + filename);
      } else {
        if (filename.size() < subfix.size()) {
          continue;
        }
        if (filename.substr(filename.length() - subfix.size()) == subfix) {
          files.push_back(path + "/" + filename);
        }
      }
    }
  }
  closedir(dir);
}

static int __lstat(const char *file, struct stat *st = nullptr) {
  struct stat lst;
  int ret = lstat(file, &lst);
  if (st) {
    *st = lst;
  }
  return ret;
}

static int __mkdir(const char *dirname) {
  if (access(dirname, F_OK) == 0) {
    return 0;
  }
  return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

bool FSUtil::Mkdir(const std::string &dirname) {
  if (__lstat(dirname.c_str()) == 0) {
    return true;
  }
  char *path = strdup(dirname.c_str());
  char *ptr = strchr(path + 1, '/');
  do {
    for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
      *ptr = '\0';
      if (__mkdir(path) != 0) {
        break;
      }
    }
    if (ptr != nullptr) {
      break;
    } else if (__mkdir(path) != 0) {
      break;
    }
    free(path);
    return true;
  } while (0);
  free(path);
  return false;
}

bool FSUtil::IsRunningPidfile(const std::string &pidfile) {
  if (__lstat(pidfile.c_str()) != 0) {
    return false;
  }
  std::ifstream ifs(pidfile);
  std::string line;
  if (!ifs || !std::getline(ifs, line)) {
    return false;
  }
  if (line.empty()) {
    return false;
  }
  pid_t pid = atoi(line.c_str());
  if (pid <= 1) {
    return false;
  }
  if (kill(pid, 0) != 0) {
    return false;
  }
  return true;
}
}  // namespace sylar