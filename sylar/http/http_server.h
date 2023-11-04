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
  typedef std::shared_ptr<HttpServer> ptr;
  HttpServer(bool keepalive = false,
             sylar::IOManager *worker = sylar::IOManager::GetThis(),
             sylar::IOManager *accepr_worker = sylar::IOManager::GetThis());
  ServletDispatch::ptr getServletDispath() const { return m_dispatch; }
  void setServletDispath(const ServletDispatch::ptr v) { m_dispatch = v; };

 protected:
  virtual void handleClient(Socket::ptr client) override;

 private:
  bool m_isKeepalive;
  ServletDispatch::ptr m_dispatch;
};
}  // namespace http
}  // namespace sylar

#endif  // SYLAR_HTTP_HTTP_SERVER_