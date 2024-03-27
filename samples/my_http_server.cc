#include "address.h"
#include "http/http_server.h"
#include "iomanager.h"
#include "log.h"
#include "sylar/http/http_server.h"
#include "sylar/log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
  g_logger->setLevel(sylar::LogLevel::INFO);
  sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
  if (!addr) {
    SYLAR_LOG_ERROR(g_logger) << "get address error";
    return;
  }
  sylar::http::HttpServer::ptr http_server(new sylar::http::HttpServer);
  while (!http_server->bind(addr)) {
    SYLAR_LOG_ERROR(g_logger) << "bind" << *addr << " fail";
    return;
  }

  http_server->start();
}

int main() {
  sylar::IOManager iom(3);
  iom.schedule(run);
  return 0;
}