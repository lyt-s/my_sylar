#include "address.h"
#include "iomanager.h"
#include "log.h"
#include "sylar/socket.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_socket() {
  sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress("www.baidu.com");

  if (addr) {
    SYLAR_LOG_INFO(g_logger) << "get address: " << addr->toString();
  } else {
    SYLAR_LOG_INFO(g_logger) << "get address fail  ";
    return;
  }

  sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
  addr->setPort(80);
  if (!sock->connect(addr)) {
    SYLAR_LOG_INFO(g_logger) << "connect " << addr->toString() << "fail";
    return;
  } else {
    SYLAR_LOG_INFO(g_logger) << "connect " << addr->toString() << "connected";
  }

  const char buff[] = "GET / HTTP/1.0\r\n\n";
  int rt = sock->send(buff, sizeof(buff));
  if (rt <= 0) {
    SYLAR_LOG_INFO(g_logger) << "send fail rt= " << rt;
    return;
  }
  std::string buffs;
  buffs.resize(4096);
  rt = sock->recv(&buffs[0], buffs.size());

  if (rt <= 0) {
    SYLAR_LOG_INFO(g_logger) << "recv fail rt= " << rt;
    return;
  }

  SYLAR_LOG_INFO(g_logger) << buffs;
}
int main() {
  sylar::IOManager iom;
  iom.schedule(&test_socket);
  return 0;
}