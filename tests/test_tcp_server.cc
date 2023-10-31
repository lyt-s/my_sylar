#include <unistd.h>
#include <memory>
#include <vector>
#include "address.h"
#include "log.h"
#include "sylar/iomanager.h"
#include "sylar/tcp_server.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
  auto addr = sylar::Address::LookupAny("0.0.0.0:8033");
  //   auto addr2 = std::make_shared<sylar::UnixAddress>("/tmp/unix_addr");
  //   SYLAR_LOG_INFO(g_logger) << *addr << *addr2;
  std::vector<sylar::Address::ptr> addrs;
  addrs.push_back(addr);
  //   addrs.push_back(addr2);
  std::vector<sylar::Address::ptr> failed_bind_addrs;
  sylar::TcpServer::ptr tcp_server(new sylar::TcpServer);
  while (!tcp_server->bind(addrs, failed_bind_addrs)) {
    sleep(2);
  }

  tcp_server->start();
}

int main() {
  sylar::IOManager iom(2);
  iom.schedule(run);
  return 0;
}