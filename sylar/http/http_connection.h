#ifndef SYLAR_HTTP_HTTP_CONNECTION_H_
#define SYLAR_HTTP_HTTP_CONNECTION_H_

#include <memory>
#include "http/http.h"
#include "socket.h"
#include "socket_stream.h"
namespace sylar {
namespace http {

class HttpConnection : public SocketStream {
 public:
  using ptr = std::shared_ptr<HttpConnection>;

  HttpConnection(Socket::ptr sock, bool owner = true);
  HttpResponse::ptr recvResponse();
  int sendRequest(HttpRequest::ptr rsp);

 private:
};
}  // namespace http
}  // namespace sylar
#endif  // SYLAR_HTTP_HTTP_CONNECTION_H_