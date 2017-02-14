// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#include <string>
#include "slash_status.h"
#include "pink_thread.h"
#include "worker_thread.h"
#include "dispatch_thread.h"
#include "http_conn.h"
class MyHttpConn : public pink::HttpConn {
 public:
  MyHttpConn(const int fd, const std::string &ip_port,
      pink::WorkerThread<MyHttpConn>* worker) :
    HttpConn(fd, ip_port) {
  }
  virtual slash::Status DealMessage(pink::HttpRequest* req, pink::HttpResponse* res) {
    std::cout << "handle get"<< std::endl;
    std::cout << " + method: " << req->method << std::endl;
    std::cout << " + path: " << req->path << std::endl;
    std::cout << " + version: " << req->version << std::endl;
    std::cout << " + content: " << req->version << std::endl;
    std::cout << " + headers: " << std::endl;
    for (auto h : req->headers) {
      std::cout << "   + " << h.first << ":" << h.second << std::endl;
    }
    std::cout << " + query_params: " << std::endl;
    for (auto q : req->query_params) {
      std::cout << "   + " << q.first << ":" << q.second << std::endl;
    }
    res->content.append("HTTP/1.1 200 OK\r\n");
    if (req->path == std::string("/country")) {
      res->content.append("Content-Length:7");
      res->content.append("\r\n\r\n");
      res->content.append("china\r\n");
      return slash::Status::OK();
    }
    return slash::Status::OK();
  }
};

int main(int argc, char* argv[]) {
  int work_num = 1;
  pink::WorkerThread<MyHttpConn>* workers_[work_num];
  for (int i = 0; i < work_num; i++) {
    workers_[i] = new pink::WorkerThread<MyHttpConn>();
  }
  std::set<std::string> ips;
  ips.insert(argv[1]);
  ips.insert("127.0.0.1");
  pink::DispatchThread<MyHttpConn> *t;
  t = new pink::DispatchThread<MyHttpConn>(ips, 8089, 1, workers_, 3000);
  t->StartThread();
  sleep(10000);
}
