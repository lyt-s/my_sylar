/*
 * @Date: 2023-11-02 21:03:30
 * @LastEditors: lyt-s 1814666859@qq.com
 * @LastEditTime: 2023-11-03 21:06:55
 * @FilePath: /my_sylar/sylar/http/servlet.h
 * @Description:
 */
#ifndef SYLAR_HTTP_SERVLET_H_
#define SYLAR_HTTP_SERVLET_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "http/http.h"
#include "http/http_session.h"
#include "thread.h"
namespace sylar {
namespace http {
class Servlet {
 public:
  using ptr = std::shared_ptr<Servlet>;
  Servlet(const std::string &name) : m_name(name) {}
  virtual ~Servlet() {}
  virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                         sylar::http::HttpResponse::ptr response,
                         sylar::http::HttpSession::ptr session) = 0;
  const std::string &getName() const { return m_name; }

 protected:
  // name 就是调试，输出作用
  std::string m_name;
};

class FunctionServlet : public Servlet {
 public:
  using ptr = std::shared_ptr<FunctionServlet>;
  using callback =
      std::function<int32_t(sylar::http::HttpRequest::ptr request,
                            sylar::http::HttpResponse::ptr response,
                            sylar::http::HttpSession::ptr session)>;
  FunctionServlet(callback cb);

  virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                         sylar::http::HttpResponse::ptr response,
                         sylar::http::HttpSession::ptr session) override;

 private:
  callback m_cb;
};

class ServletDispatch : public Servlet {
 public:
  using ptr = std::shared_ptr<ServletDispatch>;
  typedef RWMutex RWMutexTYpe;

  ServletDispatch();
  virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                         sylar::http::HttpResponse::ptr response,
                         sylar::http::HttpSession::ptr session) override;

  void addServlet(const std::string &uri, Servlet::ptr servlet);
  void addServlet(const std::string &uri, FunctionServlet::callback cb);
  void addGlobServlet(const std::string &uri, Servlet::ptr servlet);
  void addGlobServlet(const std::string &uri, FunctionServlet::callback cb);

  void delServlet(const std::string &uri);
  void delGlobServlet(const std::string &uri);

  Servlet::ptr getDefault() const { return m_default; }
  // 线程安全隐患 todo
  void setDefault(const Servlet::ptr v) { m_default = v; }

  Servlet::ptr getSetvlet(const std::string &uri);
  Servlet::ptr getGlobSetvlet(const std::string &uri);

  Servlet::ptr getMatchServlet(const std::string &uri);

 private:
  RWMutex m_mutex;
  // uri (sylar/xxx)-> servlet 精准
  std::unordered_map<std::string, Servlet::ptr> m_datas;
  // uri -(sylar/*)> servlet 模糊
  std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
  // 默认servlet ,所有路径都没匹配到时，使用
  Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet {
 public:
  typedef std::shared_ptr<NotFoundServlet> ptr;
  NotFoundServlet(const std::string &name);

  virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                         sylar::http::HttpResponse::ptr response,
                         sylar::http::HttpSession::ptr session) override;

 private:
  std::string m_name;
  std::string m_content;
};
}  // namespace http
}  // namespace sylar

#endif  // SYLAR_HTTP_SERVLET_H_