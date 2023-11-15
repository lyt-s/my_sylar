#ifndef SYLAR_HTTP_HTTP_PARSER_H_
#define SYLAR_HTTP_HTTP_PARSER_H_
#include <cstdint>
#include "sylar/http/http.h"
#include "sylar/http/http11_parser.h"
#include "sylar/http/httpclient_parser.h"

namespace sylar {
namespace http {

class HttpRequestParser {
 public:
  typedef std::shared_ptr<HttpRequestParser> ptr;
  HttpRequestParser();

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

  /**
   * @brief 解析HTTP响应协议
   * @param[in, out] data 协议数据内存
   * @param[in] len 协议数据内存大小
   * @param[in] chunck 是否在解析chunck
   * @return 返回实际解析的长度,并且移除已解析的数据
   */
  size_t execute(char *data, size_t len, bool chunck);
  int isFinished() const;
  int hasError() const;

  HttpResponse::ptr getData() const { return m_data; }
  const httpclient_parser getParser() const { return m_parser; }
  void setError(const int v) { m_error = v; }

  uint64_t getContentLength();

 public:
  static uint64_t GetHttpResponseBufferSize();
  static uint64_t GetHttpResponseMaxBodySize();

 private:
  httpclient_parser m_parser;
  HttpResponse::ptr m_data;
  // 1001: invalid version
  // 1002:invaild http request field length
  int m_error;
};
}  // namespace http
}  // namespace sylar

#endif  // SYLAR_HTTP_HTTP_PARSER_H_