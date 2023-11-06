/*
 * @Date: 2023-11-03 20:24:14
 * @LastEditors: lyt-s 1814666859@qq.com
 * @LastEditTime: 2023-11-03 21:04:53
 * @FilePath: /my_sylar/sylar/http/servlet.cc
 * @Description:
 */
#include "servlet.h"
#include <fnmatch.h>
#include <memory>
#include <string>
#include <utility>
#include "noncopyable.h"
namespace sylar {
namespace http {

FunctionServlet::FunctionServlet(callback cb)
    : Servlet("FunctionServlet"), m_cb(cb) {}

int32_t FunctionServlet::handle(sylar::http::HttpRequest::ptr request,
                                sylar::http::HttpResponse::ptr response,
                                sylar::http::HttpSession::ptr session) {
  return m_cb(request, response, session);
}

ServletDispatch::ServletDispatch() : Servlet("ServletDispatch") {
  m_default.reset(new NotFoundServlet("sylar"));
}

int32_t ServletDispatch::handle(sylar::http::HttpRequest::ptr request,
                                sylar::http::HttpResponse::ptr response,
                                sylar::http::HttpSession::ptr session) {
  auto slt = getMatchServlet(request->getPath());
  if (slt) {
    slt->handle(request, response, session);
  }
  return 0;
}

void ServletDispatch::addServlet(const std::string &uri, Servlet::ptr servlet) {
  RWMutexType::WriteLock lock(m_mutex);
  m_datas[uri] = servlet;
}
void ServletDispatch::addServlet(const std::string &uri,
                                 FunctionServlet::callback cb) {
  RWMutexType::WriteLock lock(m_mutex);
  m_datas[uri].reset(new FunctionServlet(cb));
}
void ServletDispatch::addGlobServlet(const std::string &uri,
                                     Servlet::ptr servlet) {
  RWMutexType::WriteLock lock(m_mutex);
  for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
    if (it->first == uri) {
      m_globs.erase(it);
      break;
    }
  }

  m_globs.push_back(std::make_pair(uri, servlet));
}
void ServletDispatch::addGlobServlet(const std::string &uri,
                                     FunctionServlet::callback cb) {
  return addGlobServlet(uri, std::make_shared<FunctionServlet>(cb));
}

void ServletDispatch::delServlet(const std::string &uri) {
  RWMutexType::WriteLock lock(m_mutex);
  m_datas.erase(uri);
}
void ServletDispatch::delGlobServlet(const std::string &uri) {
  RWMutexType::WriteLock lock(m_mutex);
  for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
    if (it->first == uri) {
      m_globs.erase(it);
      break;
    }
  }
}

Servlet::ptr ServletDispatch::getSetvlet(const std::string &uri) {
  RWMutexType::ReadLock lock(m_mutex);
  auto it = m_datas.find(uri);
  return it == m_datas.end() ? nullptr : it->second;
}
Servlet::ptr ServletDispatch::getGlobSetvlet(const std::string &uri) {
  RWMutexType::ReadLock lock(m_mutex);
  for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
    if (it->first == uri) {
      return it->second;
    }
  }
  return nullptr;
}
Servlet::ptr ServletDispatch::getMatchServlet(const std::string &uri) {
  RWMutexType::ReadLock lock(m_mutex);
  auto mit = m_datas.find(uri);
  if (mit != m_datas.end()) {
    return mit->second;
  }
  for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
    // todo
    if (!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
      return it->second;
    }
  }
  return m_default;
}

NotFoundServlet::NotFoundServlet(const std::string &name)
    : Servlet("NotFoundServlet"), m_name(name) {
  m_content =
      "<html><head><title>404 Not Found"
      "</title></head><body><center><h1>404 Not Found</h1></center>"
      "<hr><center>" +
      name + "</center></body></html>";
}

int32_t NotFoundServlet::handle(sylar::http::HttpRequest::ptr request,
                                sylar::http::HttpResponse::ptr response,
                                sylar::http::HttpSession::ptr session) {
  response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
  response->setHeader("Server", "sylar/1.0.0");
  response->setHeader("Content-Type", "text/html");
  response->setBody(m_content);
  return 0;
}
}  // namespace http

}  // namespace sylar