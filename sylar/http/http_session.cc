#include "sylar/http/http_session.h"
#include <strings.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include "sylar/http/http.h"
#include "sylar/http/http_parser.h"
#include "sylar/streams/socket_stream.h"

namespace sylar {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner) {}

HttpRequest::ptr HttpSession::recvRequest() {
  HttpRequestParser::ptr parser(new HttpRequestParser);
  uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
  // uint64_t buff_size = 100;
  std::shared_ptr<char> buffer(new char[buff_size],
                               [](char *ptr) { delete[] ptr; });
  char *data = buffer.get();
  int offset = 0;

  do {
    int len = read(data + offset, buff_size - offset);
    if (len <= 0) {
      close();
      return nullptr;
    }
    len += offset;
    size_t nparse = parser->execute(data, len);
    if (parser->hasError()) {
      close();
      return nullptr;
    }
    offset = len - nparse;
    if (offset == (int)buff_size) {
      close();
      return nullptr;
    }
    if (parser->isFinished()) {
      break;
    }
  } while (true);

  int64_t length = parser->getContentLength();
  if (length > 0) {
    std::string body;
    body.resize(length);

    int len = 0;

    if (length >= offset) {
      std::memcpy(&body[0], data, offset);
      len = offset;

    } else {
      memcpy(&body[0], data, length);
      len = length;
    }
    length -= offset;
    if (length > 0) {
      if (readFixSize(&body[len], length) <= 0) {
        close();
        return nullptr;
      }
    }
    parser->getData()->setBody(body);
  }

  std::string keep_alive = parser->getData()->getHeader("connection");
  if (!strcasecmp(keep_alive.c_str(), "keep-alive")) {
    parser->getData()->setClose(false);
  }
  // parser->getData()->init();
  return parser->getData();
}
int HttpSession::sendResponse(HttpResponse::ptr rsp) {
  std::stringstream ss;
  ss << *rsp;
  std::string data = ss.str();
  return writeFixSize(data.c_str(), data.size());
}
}  // namespace http
}  // namespace sylar