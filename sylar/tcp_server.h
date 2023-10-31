#ifndef SYLAR_TCP_SERVER_H_
#define SYLAR_TCP_SERVER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "address.h"
#include "iomanager.h"
#include "noncopyable.h"
#include "socket.h"

namespace sylar {
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
 public:
  using ptr = std::shared_ptr<TcpServer>;
  TcpServer(sylar::IOManager *worker = sylar::IOManager::GetThis(),
            sylar::IOManager *accept_worker = sylar::IOManager::GetThis());

  virtual ~TcpServer();

  // 一个地址
  virtual bool bind(sylar::Address::ptr addr);
  // 多个地址
  virtual bool bind(const std::vector<Address::ptr> &addrs,
                    std::vector<Address::ptr> &fail_bind_addrs);
  virtual bool start();  // 服务器启动
  virtual bool stop();

  uint64_t getRecvTimeout() const { return m_recvTimeout; }
  std::string getName() const { return m_name; }
  void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
  void setName(const std::string &v) { m_name = v; }

  bool isStop() const { return m_isStop; }

 protected:
  // 每accept一次，就会执行这个方法
  virtual void handleClient(Socket::ptr client);
  virtual void startAccept(Socket::ptr sock);

 private:
  // 同时监听多地址
  std::vector<Socket::ptr> m_socks;
  IOManager *m_worker;
  IOManager *m_accept_worker;
  // 防止被攻击，恶意请求，长时间不发消息
  uint64_t m_recvTimeout;
  std::string m_name;  // 名字 同时支持多种协议， http udp
  bool m_isStop;       // server 是不是停止了
};
}  // namespace sylar
#endif  // SYLAR_TCP_SERVER_H_