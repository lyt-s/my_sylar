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
                       sylar::IOManager *io_worker,
                       sylar::IOManager *accept_worker)
    : TcpServer(worker, io_worker, accept_worker), m_isKeepalive(keepalive) {
  m_dispatch.reset(new ServletDispatch);

  // m_type = "http";
  // m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
  // m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
}

void HttpServer::setName(const std::string &v) {
  TcpServer::setName(v);
  m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}

void HttpServer::handleClient(Socket::ptr client) {
  SYLAR_LOG_DEBUG(g_logger) << "handleClient " << *client;
  HttpSession::ptr session(new HttpSession(client));
  do {
    auto req = session->recvRequest();
    if (!req) {
      SYLAR_LOG_DEBUG(g_logger)
          << "recv http request fail, errno=" << errno
          << " errstr=" << strerror(errno) << " cliet:" << *client
          << " keep_alive=" << m_isKeepalive;
      break;
    }

    HttpResponse::ptr rsp(
        new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));
    rsp->setHeader("Server", getName());
    m_dispatch->handle(req, rsp, session);
    // rsp->setBody("hello sylar");
    // SYLAR_LOG_INFO(g_logger) << "request: " << std::endl << *req;
    // SYLAR_LOG_INFO(g_logger) << "response: " << std::endl << *rsp;
    session->sendResponse(rsp);

    if (!m_isKeepalive || req->isClose()) {
      SYLAR_LOG_DEBUG(g_logger) << "m_isKeepalive= " << m_isKeepalive;
      break;
    }
  } while (m_isKeepalive);
  session->close();
}
}  // namespace http

}  // namespace sylar