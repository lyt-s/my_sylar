#include "http_server.h"
#include <cerrno>
#include <cstring>
#include <memory>
#include "http/http.h"
#include "http/http_session.h"
#include "http/servlet.h"
#include "log.h"
#include "tcp_server.h"

namespace sylar {
namespace http {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive, sylar::IOManager *worker,
                       sylar::IOManager *accepr_worker)
    : TcpServer(worker, accepr_worker), m_isKeepalive(keepalive) {
  m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr client) {
  sylar::http::HttpSession::ptr session = std::make_shared<HttpSession>(client);
  do {
    auto req = session->recvRequest();
    if (!req) {
      SYLAR_LOG_WARN(g_logger)
          << "recv http request fail, errno=" << errno
          << " errstr=" << strerror(errno) << " client:" << *client;
      break;
    }
    HttpResponse::ptr rsp = std::make_shared<HttpResponse>(
        req->getVersion(), req->isClose() || !m_isKeepalive);

    m_dispatch->handle(req, rsp, session);
    // rsp->setBody("hello sylar");
    // SYLAR_LOG_INFO(g_logger) << "request: " << std::endl << *req;
    // SYLAR_LOG_INFO(g_logger) << "response: " << std::endl << *rsp;
    session->sendResponse(rsp);

  } while (m_isKeepalive);
  session->close();
}
}  // namespace http

}  // namespace sylar