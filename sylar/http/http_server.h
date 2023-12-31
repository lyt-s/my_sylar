#ifndef SYLAR_HTTP_HTTP_SERVER_
#define SYLAR_HTTP_HTTP_SERVER_

#include <memory>
#include "http/servlet.h"
#include "iomanager.h"
#include "tcp_server.h"
namespace sylar {
namespace http {
class HttpServer : public TcpServer {
 public:
  /// 智能指针类型
  typedef std::shared_ptr<HttpServer> ptr;

  /**
   * @brief 构造函数
   * @param[in] keepalive 是否长连接
   * @param[in] worker 工作调度器
   * @param[in] accept_worker 接收连接调度器
   */
  HttpServer(bool keepalive = false,
             sylar::IOManager *worker = sylar::IOManager::GetThis(),
             sylar::IOManager *io_worker = sylar::IOManager::GetThis(),
             sylar::IOManager *accept_worker = sylar::IOManager::GetThis());

  /**
   * @brief 获取ServletDispatch
   */
  ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }

  /**
   * @brief 设置ServletDispatch
   */
  void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

  virtual void setName(const std::string &v) override;

 protected:
  virtual void handleClient(Socket::ptr client) override;

 private:
  bool m_isKeepalive;
  ServletDispatch::ptr m_dispatch;
};
}  // namespace http
}  // namespace sylar

#endif  // SYLAR_HTTP_HTTP_SERVER_