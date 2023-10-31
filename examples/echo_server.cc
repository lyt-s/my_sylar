#include <bits/types/struct_iovec.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>
#include "sylar/address.h"
#include "sylar/bytearray.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/socket.h"
#include "sylar/tcp_server.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

class EchoServer : public sylar::TcpServer {
 public:
  EchoServer(int type);
  void handleClient(sylar::Socket::ptr client);

 private:
  int m_type = 0;
};

EchoServer::EchoServer(int type) : m_type(type) {}

void EchoServer::handleClient(sylar::Socket::ptr client) {
  SYLAR_LOG_INFO(g_logger) << "handleClient" << &client;
  sylar::ByteArray::ptr ba = std::make_shared<sylar::ByteArray>();
  while (true) {
    ba->clear();
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, 1024);

    int rt = client->recv(&iovs[0], iovs.size());
    SYLAR_LOG_INFO(g_logger) << "recv rt=" << rt;
    if (rt == 0) {
      SYLAR_LOG_INFO(g_logger) << " client close:" << *client;
      break;
    } else if (rt < 0) {
      SYLAR_LOG_INFO(g_logger) << "client error rt=" << rt << " errno=" << errno
                               << " errstr=" << strerror(errno);
      break;
    }

    ba->setPosition(ba->getPosition() + rt);
    ba->setPosition(0);
    if (m_type == 1) {  // text
      //   SYLAR_LOG_INFO(g_logger) << ba->toString();
      std::cout << ba->toString();
    } else {
      //   SYLAR_LOG_INFO(g_logger) << ba->toHexString();
      std::cout << ba->toHexString();
    }
    // todo
    std::cout.flush();
  }
}

int type = 1;

void run() {
  EchoServer::ptr es(new EchoServer(type));
  auto addr = sylar::Address::LookupAny("0.0.0.0:8020");
  while (!es->bind(addr)) {
    sleep(2);
  }
  es->start();
}

int main(int argc, char **argv) {
  if (argc < 2) {
    SYLAR_LOG_INFO(g_logger)
        << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
    return 0;
  }
  if (!strcmp(argv[1], "-b")) {
    type = 2;
  }
  sylar::IOManager iom(2);
  iom.schedule(run);
  return 0;
}