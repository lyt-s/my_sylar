#ifndef SYLAR_HTTP_HTTP_SESSION_H_
#define SYLAR_HTTP_HTTP_SESSION_H_

#include <memory>
#include "http/http.h"
#include "socket.h"
#include "socket_stream.h"
namespace sylar {
namespace http {

class HttpSession : public SocketStream {
 public:
  using ptr = std::shared_ptr<HttpSession>;

  HttpSession(Socket::ptr sock, bool owner = true);
  HttpRequest::ptr recvRequest();
  int sendResponse(HttpResponse::ptr rsp);

 private:
};
}  // namespace http
}  // namespace sylar
#endif  // SYLAR_HTTP_HTTP_SESSION_H_