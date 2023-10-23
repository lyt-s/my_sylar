#ifndef FD_MANAGER_H_
#define FD_MANAGER_H_

#include <cstdint>
#include <memory>
#include <vector>
#include "iomanager.h"
#include "singleton.h"
#include "thread.h"

namespace sylar {

/**
 * @brief 文件句柄上下文类
 * @details 管理文件句柄类型(是否socket)
 *          是否阻塞,是否关闭,读/写超时时间
 */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
 public:
  typedef std::shared_ptr<FdCtx> ptr;
  /**
   * @brief 通过文件句柄构造FdCtx
   */
  FdCtx(int fd);
  /**
   * @brief 析构函数
   */
  ~FdCtx();

  bool init();
  bool isInit() const { return m_isInit; }
  bool isSocket() const { return m_isSocket; }
  bool isClosed() const { return m_isClosed; }
  bool close();

  void setUserNonblock(bool v) { m_userNonblock = v; }
  bool getUserNonblock() const { return m_userNonblock; }

  void setSysNonblock(bool v) { m_sysNonblock = v; }
  bool getSysNonblock() const { return m_sysNonblock; }

  void setTimeout(int type, uint64_t v);
  uint64_t getTimeout(int type);

 private:
  /// 是否初始化
  bool m_isInit : 1;
  /// 是否socket
  bool m_isSocket : 1;
  /// 是否hook非阻塞
  bool m_sysNonblock : 1;
  /// 是否用户主动设置非阻塞
  bool m_userNonblock : 1;
  /// 是否关闭
  bool m_isClosed : 1;
  /// 文件句柄
  int m_fd;
  /// 读超时时间毫秒
  uint64_t m_recvTimeout;
  /// 写超时时间毫秒
  uint64_t m_sendTimeout;
  // sylar::IOManager *m_iomanager;
};

class FdManager {
 public:
  typedef RWMutex RWMutexType;
  FdManager();

  FdCtx::ptr get(int fd, bool auto_create = false);
  void del(int fd);

 private:
  RWMutexType m_mutex;
  std::vector<FdCtx::ptr> m_datas;
};

// 单例
typedef Singleton<FdManager> FdMgr;
}  // namespace sylar
#endif  // FD_MANAGER_H_