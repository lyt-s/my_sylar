#include "http_session.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include "http/http.h"
#include "http/http_parser.h"
#include "socket_stream.h"

namespace sylar {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner) {}

HttpRequest::ptr HttpSession::recvRequest() {
  HttpRequestParse::ptr parser(new HttpRequestParse);
  uint64_t buffer_size = HttpRequestParse::GetHttpRequestBufferSize();
  std::shared_ptr<char> buffer(
      new char[HttpRequestParse::GetHttpRequestBufferSize()],
      [](char *ptr) { delete[] ptr; });

  char *data = buffer.get();
  int offset = 0;

  do {
    int len = read(data + offset, buffer_size - offset);
    if (len <= 0) {
      return nullptr;
    }
    len += offset;
    size_t nparser = parser->execute(data, len + offset);
    if (parser->hasError()) {
      return nullptr;
    }
    offset = len - nparser;
    if (offset == (int64_t)buffer_size) {
      return nullptr;
    }
    if (parser->isFinished()) {
      // todo
      break;
    }
  } while (true);

  int64_t length = parser->getContentLength();
  if (length > 0) {
    std::string body;
    body.reserve(length);
    if (length >= offset) {
      body.append(data, offset);
    } else {
      body.append(data, length);
    }
    length -= offset;
    if (length > 0) {
      if (readFixSize(&body[body.size()], length) <= 0) return nullptr;
    }
    parser->getData()->setBody(body);
  }
  return parser->getData();
}
int HttpSession::sendResponse(HttpResponse::ptr rsp) {
  std::stringstream ss;
  ss << *rsp;
  std::string data = ss.str();
  return writeFixSize(ss.str().c_str(), data.size());
}
}  // namespace http
}  // namespace sylar