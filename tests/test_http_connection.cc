
#include "iomanager.h"
#include "log.h"
#include "sylar/http/http_connection.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
  sylar::Address::ptr addr =
      sylar::Address::LookupAnyIPAddress("www.sylar.top:80");
  if (!addr) {
    SYLAR_LOG_INFO(g_logger) << "get addr error";
    return;
  }

  sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
  bool rt = sock->connect(addr);
  if (!rt) {
    SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " failed";
    return;
  }
  sylar::http::HttpConnection::ptr conn(new sylar::http::HttpConnection(sock));
  sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
  req->setPath("/blog/");
  req->setHeader("host", "www.sylar.top");
  SYLAR_LOG_INFO(g_logger) << "req:" << std::endl << *req;

  conn->sendRequest(req);
  auto rsp = conn->recvResponse();

  if (!rsp) {
    SYLAR_LOG_INFO(g_logger) << "recv response error";
    return;
  }
  SYLAR_LOG_INFO(g_logger) << "rsp:" << std::endl << *rsp;

  std::ofstream ofs("rsp.dat");
  ofs << *rsp;

  SYLAR_LOG_INFO(g_logger) << "=========================";

  // auto r =
  //     sylar::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
  // SYLAR_LOG_INFO(g_logger) << "result=" << r->result << " error=" << r->error
  //                          << " rsp="
  //                          << (r->response ? r->response->toString() : "");

  // SYLAR_LOG_INFO(g_logger) << "=========================";
  // test_pool();
}
int main() {
  sylar::IOManager iom(1);
  iom.schedule(run);
  return 0;
}