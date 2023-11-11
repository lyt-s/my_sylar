#ifndef SYLAR_APPLIACTION_H_
#define SYLAR_APPLIACTION_H_
#include <map>
#include "http/http_server.h"
#include "iomanager.h"
#include "tcp_server.h"
namespace sylar {
class Application {
 public:
  Application();

  static Application *GetInstance() { return s_instance; }
  bool init(int argc, char **argv);
  bool run();

 private:
  int main(int argc, char **argv);
  int run_fiber();

 private:
  int m_argc = 0;
  char **m_argv = nullptr;

  std::vector<sylar::http::HttpServer::ptr> m_httpservers;
  // std::map<std::string, std::vector<TcpServer::ptr> > m_servers;
  IOManager::ptr m_mainIOManager;
  static Application *s_instance;
};
}  // namespace sylar

#endif  // SYLAR_APPLIACTION_H_