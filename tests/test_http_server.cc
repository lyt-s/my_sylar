#include <memory>
#include "address.h"
#include "http/http.h"
#include "http/http_session.h"
#include "iomanager.h"
#include "sylar/http/http_server.h"
#include "sylar/http/servlet.h"

#define XX(...) #__VA_ARGS__

void run() {
  sylar::http::HttpServer::ptr server =
      std::make_shared<sylar::http::HttpServer>();
  sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
  while (!server->bind(addr)) {
    sleep(2);
  }
  auto sd = server->getServletDispath();
  sd->addServlet("/sylar/xx", [](sylar::http::HttpRequest::ptr req,
                                 sylar::http::HttpResponse::ptr rsp,
                                 sylar::http::HttpSession::ptr session) {
    rsp->setBody(req->toString());
    return 0;
  });

  sd->addGlobServlet("/sylar/*", [](sylar::http::HttpRequest::ptr req,
                                    sylar::http::HttpResponse::ptr rsp,
                                    sylar::http::HttpSession::ptr session) {
    rsp->setBody("Glob:\r\n" + req->toString());
    return 0;
  });

  // sd->addGlobServlet("/sylarx/*", [](sylar::http::HttpRequest::ptr req,
  //                                    sylar::http::HttpResponse::ptr rsp,
  //                                    sylar::http::HttpSession::ptr session) {
  //   rsp->setBody(
  //       XX(<html><head><title> 404 Not Found</ title></ head><body><center>
  //                  <h1> 404 Not Found</ h1></ center><hr><center>
  //                      nginx /
  //                  1.16.0 <
  //              / center > </ body></ html> < !--a padding to disable MSIE and
  //          Chrome friendly error page-- > < !--a padding to disable MSIE and
  //          Chrome friendly error page-- > < !--a padding to disable MSIE and
  //          Chrome friendly error page-- > < !--a padding to disable MSIE and
  //          Chrome friendly error page-- > < !--a padding to disable MSIE and
  //          Chrome friendly error page-- > < !--a padding to disable MSIE and
  //          Chrome friendly error page-- >));
  //   return 0;
  // });

  server->start();
}
int main(int argc, char **argv) {
  sylar::IOManager iom(2);
  iom.schedule(run);
  return 0;
}