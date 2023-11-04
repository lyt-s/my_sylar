#include <iostream>
#include "sylar/http/http.h"
#include "sylar/http/http_parser.h"
#include "sylar/stream.h"
void test() {
  sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
  req->setHeader("host", "www.sylar.top");
  req->setBody("hello sylar");

  req->dump(std::cout) << std::endl;
}

void test_response() {
  sylar::http::HttpResponse::ptr rsp(new sylar::http::HttpResponse);
  rsp->setHeaders("X-X", "sylar");
  rsp->setBody("hello sylar");
  rsp->setClose(false);
  rsp->setStatus((sylar::http::HttpStatus)(400));

  rsp->dump(std::cout) << std::endl;
}

int main(int args, char **argv) {
  test();
  test_response();
  return 0;
}