#include "tcp_server.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>
#include "address.h"
#include "config.h"
#include "log.h"
#include "socket.h"

namespace sylar {

// todo config
static sylar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    sylar::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
                          "tcp server read timeout");
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

TcpServer::TcpServer(sylar::IOManager *worker, sylar::IOManager *accept_worker)
    : m_worker(worker),
      m_accept_worker(accept_worker),
      m_recvTimeout(g_tcp_server_read_timeout->getValue()),
      m_name("sylar/1.0.0"),
      m_isStop(true) {}

TcpServer::~TcpServer() {
  for (auto &i : m_socks) {
    // socket 析构自己会close
    i->close();
  }
  m_socks.clear();
}

// 一个地址
bool TcpServer::bind(sylar::Address::ptr addr) {
  std::vector<Address::ptr> addrs;
  std::vector<Address::ptr> fail_bind_addrs;
  addrs.push_back(addr);
  return bind(addrs, fail_bind_addrs);
}
// 多个地址, 这里bind 不止bind 还有 listen
bool TcpServer::bind(const std::vector<Address::ptr> &addrs,
                     std::vector<Address::ptr> &fail_bind_addrs) {
  for (auto &addr : addrs) {
    Socket::ptr sock = Socket::CreateTCP(addr);
    if (!sock->bind(addr)) {
      SYLAR_LOG_ERROR(g_logger)
          << "bind fail errno=" << errno << " errstr=" << strerror(errno)
          << " addr=[" << addr->toString() << "]";
      fail_bind_addrs.push_back(addr);
      continue;
    }
    if (!sock->listen()) {
      SYLAR_LOG_ERROR(g_logger)
          << "listen fail errno=" << errno << " errstr=" << strerror(errno)
          << " addr=[" << addr->toString() << "]";
      fail_bind_addrs.push_back(addr);
      continue;
    }
    m_socks.push_back(sock);
  }
  if (!fail_bind_addrs.empty()) {
    m_socks.clear();
    return false;
  }

  for (auto &i : m_socks) {
    SYLAR_LOG_INFO(g_logger) << "server bind success: " << *i;
  }
  return true;
}

void TcpServer::startAccept(Socket::ptr sock) {
  // server 未停止
  while (!m_isStop) {
    Socket::ptr client = sock->accept();
    if (client) {
      client->setRecvTimeout(m_recvTimeout);
      // shared_from_this(),就是拿出自己的智能指针，为什么要用？
      // -- 我handleClient结束之前，这个TcpServer
      // 不能释放，所以要把自己的引用传递进去
      // todo
      m_worker->schedule(
          std::bind(&TcpServer::handleClient, shared_from_this(), client));
    } else {
      SYLAR_LOG_ERROR(g_logger)
          << "accept errno=" << errno << " errstr=" << strerror(errno);
    }
  }
}
// 服务器启动
bool TcpServer::start() {
  if (!m_isStop) {
    return true;
  }

  m_isStop = false;
  for (auto &sock : m_socks) {
    m_accept_worker->schedule(
        std::bind(&TcpServer::startAccept, shared_from_this(), sock));
  }
  return true;
}

bool TcpServer::stop() {
  m_isStop = true;
  auto self = shared_from_this();
  // shared_from_this 拿去引用，防止在stop的时候 TcpServer被释放掉
  // todo
  m_accept_worker->schedule([this, self]() {
    for (auto &sock : m_socks) {
      // 在accept 不把事件取消，他不会唤醒
      sock->cancelAll();
      sock->close();
    }
    m_socks.clear();
  });
  return true;
}
void TcpServer::handleClient(Socket::ptr client) {
  SYLAR_LOG_INFO(g_logger) << "handle_client: " << *client;
}
}  // namespace sylar