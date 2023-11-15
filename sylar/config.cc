/*
 * @Date: 2023-10-21 22:24:04
 * @LastEditors: lyt-s 1814666859@qq.com
 * @LastEditTime: 2023-10-25 19:21:31
 * @FilePath: /my_sylar/sylar/config.cc
 * @Description:
 */
#include "sylar/config.h"
#include <sys/stat.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "sylar/env.h"
#include "sylar/log.h"
#include "sylar/thread.h"
#include "sylar/util.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"
namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
  RWMutexType::ReadLock lock(GetMutex());
  auto it = GetDatas().find(name);
  return it == GetDatas().end() ? nullptr : it->second;
}

/**
 * @brief
 *
 * @param prefix
 * @param node
 * @param output
 */
static void ListAllMember(
    const std::string &prefix, const YAML::Node &node,
    std::list<std::pair<std::string, const YAML::Node>> &output) {
  if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._12345678") !=
      std::string::npos) {
    SYLAR_LOG_ERROR(g_logger)
        << "config invalid name: " << prefix << " : " << node;
    return;
  }
  output.push_back(std::make_pair(prefix, node));
  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      ListAllMember(prefix.empty() ? it->first.Scalar()
                                   : prefix + "." + it->first.Scalar(),
                    it->second, output);
    }
  }
}

/**
 * @brief
 *
 * @param root
 */
void Config::LoadFromYaml(const YAML::Node &root) {
  std::list<std::pair<std::string, const YAML::Node>> all_nodes;
  ListAllMember("", root, all_nodes);

  for (auto &i : all_nodes) {
    std::string key = i.first;
    if (key.empty()) {
      continue;
    }

    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    ConfigVarBase::ptr var = LookupBase(key);

    if (var) {
      if (i.second.IsScalar()) {
        var->fromString(i.second.Scalar());
      } else {
        std::stringstream ss;
        ss << i.second;
        var->fromString(ss.str());
      }
    }
  }
}

static std::map<std::string, uint64_t> s_file2modifytime;
static sylar::Mutex s_mutex;

void Config::LoadFromConfDir(const std::string &path, bool force) {
  std::string absoulate_path =
      sylar::EnvMgr::GetInstance()->getAbsolutePath(path);
  std::vector<std::string> files;
  FSUtil::ListAllFile(files, absoulate_path, ".yml");

  for (auto &i : files) {
    // todo  // 实现了 防止重复加载
    // 这里只会检查根据文件的更改时间，来更新，不会检测实际上内容更新了没有
    {
      struct stat st;
      lstat(i.c_str(), &st);
      sylar::Mutex::Lock lock(s_mutex);
      if (s_file2modifytime[i] == (uint64_t)st.st_mtime) {
        continue;
      }
      s_file2modifytime[i] = st.st_mtime;
    }

    try {
      YAML::Node root = YAML::LoadFile(i);
      LoadFromYaml(root);
      SYLAR_LOG_INFO(g_logger) << "LoadConfFile file=" << i << " ok";
    } catch (...) {
      SYLAR_LOG_ERROR(g_logger) << "LoadConfFile file=" << i << " failed";
    }
  }
}

/**
 * @brief 遍历配置模块里面所有配置项
 * @param[in] cb 配置项回调函数
 */
void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
  RWMutexType::ReadLock lock(GetMutex());
  ConfigVarMap &m = GetDatas();
  for (auto it = m.begin(); it != m.end(); ++it) {
    cb(it->second);
  }
}
}  // namespace sylar
