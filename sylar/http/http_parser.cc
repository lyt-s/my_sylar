#include "http_parser.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include "config.h"
#include "http/http.h"
#include "http/http11_parser.h"
#include "http/httpclient_parser.h"
#include "log.h"

namespace sylar {
namespace http {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static sylar::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    sylar::Config::Lookup("http.request.buffer_size",
                          static_cast<uint64_t>(4 * 1024ull),
                          "http request buffer size");

static sylar::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    sylar::Config::Lookup("http.request.max_body_size",
                          static_cast<uint64_t>(64 * 1024 * 1024ull),
                          "http request max_body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
struct _RequestSizeIniter {
  _RequestSizeIniter() {
    // getValue 加锁
    s_http_request_buffer_size = g_http_request_buffer_size->getValue();
    s_http_request_max_body_size = g_http_request_max_body_size->getValue();

    g_http_request_buffer_size->addListener(
        [](const uint64_t &ov, const uint64_t &nv) {
          s_http_request_max_body_size = nv;
        });
  }
};

// 进main函数之前 就注册好了
static _RequestSizeIniter _init;

void on_request_method(void *data, const char *at, size_t length) {
  HttpRequestParse *parser = static_cast<HttpRequestParse *>(data);
  // 数据的起始位置，如果string的话，还要复制一份
  HttpMethod m = CharsToHttpMethod(at);

  if (m == HttpMethod::INVALID_METHOD) {
    // 出错
    SYLAR_LOG_WARN(g_logger)
        << "invalid http request method " << std::string(at, length);
    parser->setError(1000);
    return;
  }
  parser->getData()->setMethod(m);
}
void on_request_url(void *data, const char *at, size_t length) {}
void on_request_fragment(void *data, const char *at, size_t length) {
  HttpRequestParse *parser = static_cast<HttpRequestParse *>(data);
  parser->getData()->setFragment(std::string(at, length));
}
void on_request_path(void *data, const char *at, size_t length) {
  HttpRequestParse *parser = static_cast<HttpRequestParse *>(data);
  parser->getData()->setPath(std::string(at, length));
}
void on_request_query(void *data, const char *at, size_t length) {
  HttpRequestParse *parser = static_cast<HttpRequestParse *>(data);
  parser->getData()->setQuery(std::string(at, length));
}
void on_request_version(void *data, const char *at, size_t length) {
  HttpRequestParse *parser = static_cast<HttpRequestParse *>(data);
  uint8_t v = 0;
  if (strncmp(at, "HTTP/1.1", length) == 0) {
    v = 0x11;
  } else if (strncmp(at, "HTTP/1.0", length) == 0) {
    v = 0x10;
  } else {
    SYLAR_LOG_WARN(g_logger)
        << "invalid http request version:" << std::string(at, length);
    parser->setError(1001);

    return;
  }
  parser->getData()->setVersion(v);
}
void on_request_header_done(void *data, const char *at, size_t length) {
  //   HttpRequestParse *parser = static_cast<HttpRequestParse *>(data);
}
void on_request_http_field(void *data, const char *field, size_t flen,
                           const char *value, size_t vlen) {
  HttpRequestParse *parser = static_cast<HttpRequestParse *>(data);
  if (flen == 0) {
    SYLAR_LOG_WARN(g_logger) << "invaild http request field length ==0";
    parser->setError(1002);
    return;
  }
  parser->getData()->setHeader(std::string(field, flen),
                               std::string(value, vlen));
}

HttpRequestParse::HttpRequestParse() : m_error(0) {
  m_data.reset(new sylar::http::HttpRequest);
  http_parser_init(&m_parser);
  m_parser.request_method = on_request_method;
  m_parser.request_uri = on_request_url;
  m_parser.fragment = on_request_fragment;
  m_parser.request_path = on_request_path;
  m_parser.query_string = on_request_query;
  m_parser.http_version = on_request_version;
  m_parser.header_done = on_request_header_done;
  m_parser.http_field = on_request_http_field;
  // 回调时通过data取道类
  m_parser.data = this;
}

uint64_t HttpRequestParse::getContentLength() {
  return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

// 解析
// 1: 成功
// -1： 失败有错误
// >0:已处理的字节数，且data有效数据为len -v
size_t HttpRequestParse::execute(char *data, size_t len) {
  size_t offset = http_parser_execute(&m_parser, data, len, 0);
  // 没有解析完
  // todo
  std::memmove(data, data + offset, (len - offset));
  // 实际parser的数量
  return offset;
}
int HttpRequestParse::isFinished() const {
  return http_parser_finish(const_cast<http_parser *>(&m_parser));
}
int HttpRequestParse::hasError() const {
  return m_error || http_parser_has_error(const_cast<http_parser *>(&m_parser));
}

void on_response_reason(void *data, const char *at, size_t length) {
  HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
  parser->getData()->setReason(std::string(at, length));
}
void on_response_status(void *data, const char *at, size_t length) {
  HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
  HttpStatus status = static_cast<HttpStatus>(atoi(at));
  parser->getData()->setStatus(status);
}
void on_response_chunk(void *data, const char *at, size_t length) {}
void on_response_version(void *data, const char *at, size_t length) {
  HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
  uint8_t v = 0;
  if (strncmp(at, "HTTP/1.1", length) == 0) {
    v = 0x11;
  } else if (strncmp(at, "HTTP/1.0", length) == 0) {
    v = 0x10;
  } else {
    SYLAR_LOG_WARN(g_logger)
        << "invaild http response version:" << std::string(at, length);
    parser->setError(1001);
    return;
  }
  parser->getData()->setVersion(v);
}
void on_response_header_done(void *data, const char *at, size_t length) {}
void on_response_last_chunk(void *data, const char *at, size_t length) {}

void on_response_http_field(void *data, const char *field, size_t flen,
                            const char *value, size_t vlen) {
  HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
  if (flen == 0) {
    SYLAR_LOG_WARN(g_logger) << "invaild http response field length ==0";
    parser->setError(1002);
    return;
  }
  parser->getData()->setHeaders(std::string(field, flen),
                                std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser() : m_error(0) {
  m_data.reset(new sylar::http::HttpResponse);
  httpclient_parser_init(&m_parser);
  m_parser.reason_phrase = on_response_reason;
  m_parser.status_code = on_response_status;
  m_parser.chunk_size = on_response_chunk;
  m_parser.http_version = on_response_version;
  m_parser.header_done = on_response_header_done;
  m_parser.last_chunk = on_response_last_chunk;
  m_parser.http_field = on_response_http_field;
  // todo  未添加时，bug
  m_parser.data = this;
}

size_t HttpResponseParser::execute(char *data, size_t len) {
  size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
  memmove(data, data + offset, (len - offset));
  return offset;
}
int HttpResponseParser::isFinished() const {
  return httpclient_parser_is_finished(
      const_cast<httpclient_parser *>(&m_parser));
}
int HttpResponseParser::hasError() const {
  return m_error || httpclient_parser_has_error(
                        const_cast<httpclient_parser *>(&m_parser));
}

uint64_t HttpResponseParser::getContentLength() {
  return m_data->getHeaderAs<uint64_t>("content-length", 0);
}
}  // namespace http
}  // namespace sylar