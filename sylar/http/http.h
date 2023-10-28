#ifndef SYLAR_HTTP_H_
#define SYLAR_HTTP_H_

#include <sys/types.h>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include "boost/lexical_cast.hpp"
namespace sylar {
namespace http {

enum HttpMethod {
#define XX(num, bane, string) HTTP_##name = num, HTTP_METHOD_MAP(XX)
#undef XX
  HTT_INVALID_METHOD

};

enum HttpStatus {
#define XX(code, name, desc) HTTP_##name = code, HTTP_METHOD_MAP(XX)
#undef XX
};

HttpMethod StringToHttpMethod(const std::string &m);

HttpMethod CharsToHttpMethod(const char *m);

const char *HttpMethodToString(const HttpMethod &m);

const char *HttpMethodToString(const HttpStatus &s);

struct CaseInsensitiveLess {
  // 比较函数
  bool operator()(const std::string &lhs, const std::string &rhs) const;
};

class HttpRequest {
 public:
  typedef std::shared_ptr<HttpRequest> ptr;
  typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

  HttpRequest(uint8_t version = 0x11, bool close = true);

  HttpMethod getMethod() const { return m_method; }
  uint8_t getVersion() const { return m_version; }
  HttpStatus getStatus() const { return m_status; }
  const std::string &getPath() const { return m_path; }
  const std::string &getQuery() const { return m_query; }
  const std::string &getFragment() const { return m_fragment; }
  const std::string &getBody() const { return m_body; }

  const MapType &getHeaders() const { return m_headers; };
  const MapType &getParams() const { return m_params; };
  const MapType &getCookie() const { return m_cookies; };

  void setMethod(HttpMethod v) { m_method = v; }
  void setStatus(HttpStatus v) { m_status = v; }
  void setVersion(uint8_t v) { m_version = v; }

  void setPath(const std::string &v) { m_path = v; }
  void setQuery(const std::string &v) { m_query = v; }
  void setFragment(const std::string &v) { m_fragment = v; }
  void setBody(const std::string &v) { m_body = v; }

  void setHeaders(const MapType &v) { m_headers = v; };
  void setParams(const MapType &v) { m_params = v; };
  void setCookie(const MapType &v) { m_cookies = v; };

  std::string getHeader(const std::string &key, const std::string &def = "") const;
  std::string getParam(const std::string &key, const std::string &def = "") const;
  std::string getCookie(const std::string &key, const std::string &def = "") const;

  void setHeader(const std::string &key, const std::string &val);
  void setParam(const std::string &key, const std::string &val);
  void setCookie(const std::string &key, const std::string &val);

  void detHeader(const std::string &key);
  void detParam(const std::string &key);
  void detCookie(const std::string &key);

  bool hasHeader(const std::string &key, std::string *val = nullptr);
  bool hasParam(const std::string &key, std::string *val = nullptr);
  bool hasCookie(const std::string &key, std::string *val = nullptr);

 private:
  template <class T>
  bool getAs(const MapType &m, const std::string &key, T &val, const T &def = T()) {
    std::string str;
    auto it = m.find(key);
    if (it == m.end()) {
      val = def;
      return false;
    }
    try {
      val = boost::lexical_cast<T>(it->second);
      return true;
    } catch (...) {
      val = def;
    }
    return false;
  }

  template <class T>
  T getAs(const MapType &m, const std::string &key, const T &def = T()) {
    auto it = m.find(key);
    if (it == m.end()) {
      return def;
    }
    try {
      return boost::lexical_cast<T>(it->second);
    } catch (...) {
    }
    return def;
  }
  HttpMethod m_method;
  HttpStatus m_status;
  uint8_t m_version;  // 0x11 - 1.1, 0x10 -- 1.0
  bool m_close;       // 1.1 支持长连接

  std::string m_path;
  std::string m_query;
  std::string m_fragment;
  std::string m_body;
  // 重载map 比较函数
  MapType m_headers;
  MapType m_params;
  MapType m_cookies;
};
}  // namespace http
}  // namespace sylar
#endif  // SYLAR_HTTP_H_