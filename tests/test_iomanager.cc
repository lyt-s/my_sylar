#include "sylar/iomanager.h"
#include "sylar/log.h"

#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int sock = 0;

void test_fiber() {
  SYLAR_LOG_INFO(g_logger) << "test_fiber sock= " << sock;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sock, F_SETFL, O_NONBLOCK);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  inet_pton(AF_INET, "10.252.1.24", &addr.sin_addr.s_addr);

  if (!connect(sock, (const sockaddr *)&addr, sizeof(addr))) {
  } else if (errno == EINPROGRESS) {
    SYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
    sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::READ,
                                          []() { SYLAR_LOG_INFO(g_logger) << "read callback"; });
    sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::WRITE, []() {
      SYLAR_LOG_INFO(g_logger) << "write callback";
      // close(sock);
      //   sylar::IOManager::GetThis()->cancelEvent(sock, sylar::IOManager::READ);
      //   close(sock);
    });

  } else {
    SYLAR_LOG_INFO(g_logger) << "else" << errno << " " << strerror(errno);
  }
}
void test_one() {
  std::cout << "EPOLLIN= " << EPOLLIN;
  std::cout << "EPOLLOUT= " << EPOLLOUT << "\n";
  sylar::IOManager iom;
  iom.schedule(&test_fiber);
}
int main() {
  test_one();
  return 0;
}