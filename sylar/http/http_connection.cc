#include "http_connection.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include "http/http.h"
#include "http/http_parser.h"
#include "socket_stream.h"

namespace sylar {
namespace http {

HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner) {}

HttpResponse::ptr HttpConnection::recvResponse() {
  HttpResponseParser::ptr parser(new HttpResponseParser);
  // uint64_t buffer_size = HttpRequestParse::GetHttpRequestBufferSize();

  uint64_t buffer_size = 100;
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
    body.resize(length);

    int len = 0;

    if (length >= offset) {
      std::memcpy(&body[0], data, offset);
      len = offset;

    } else {
      std::memcpy(&body[0], data, length);
      len = offset;
    }
    length -= offset;
    if (length > 0) {
      if (readFixSize(&body[len], length) <= 0) return nullptr;
    }
    parser->getData()->setBody(body);
  }
  return parser->getData();
}
int HttpConnection::sendRequest(HttpRequest::ptr rsp) {
  std::stringstream ss;
  ss << *rsp;
  std::string data = ss.str();
  return writeFixSize(ss.str().c_str(), data.size());
}
}  // namespace http
}  // namespace sylar