#ifndef SYLAR_HTTP_PARSER_H_
#define SYLAR_HTTP_PARSER_H_
#include <cstdint>
#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace sylar {
namespace http {

class HttpRequestParse {
 public:
  typedef std::shared_ptr<HttpRequestParse> ptr;
  HttpRequestParse();

  size_t execute(char *data, size_t len);
  int isFinished() const;
  int hasError() const;

  HttpRequest::ptr getData() const { return m_data; }
  void setError(const int v) { m_error = v; }

  uint64_t getContentLength();

 public:
  static uint64_t GetHttpRequestBufferSize();
  static uint64_t GetHttpRequestMaxBodySize();

 private:
  http_parser m_parser;
  HttpRequest::ptr m_data;
  // 1000: invalid method
  // 1001: invalid version
  // 1002:invaild http request field length
  int m_error;
};

class HttpResponseParser {
 public:
  typedef std::shared_ptr<HttpResponseParser> ptr;
  HttpResponseParser();

  size_t execute(char *data, size_t len);
  int isFinished() const;
  int hasError() const;

  HttpResponse::ptr getData() const { return m_data; }
  void setError(const int v) { m_error = v; }

  uint64_t getContentLength();

 private:
  httpclient_parser m_parser;
  HttpResponse::ptr m_data;
  // 1001: invalid version
  // 1002:invaild http request field length
  int m_error;
};
}  // namespace http
}  // namespace sylar

#endif  // SYLAR_HTTP_PARSER_H_