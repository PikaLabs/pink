// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <string>

#include "slash/include/slash_status.h"
#include "pink/include/pink_thread.h"
#include "pink/src/worker_thread.h"
#include "pink/src/dispatch_thread.h"
#include "pink/include/http_conn.h"

using namespace pink;

class MyHttpConn : public pink::HttpConn {
 public:
  MyHttpConn(const int fd, const std::string& ip_port, Thread* worker) :
    HttpConn(fd, ip_port, worker) {
  }
  virtual void DealMessage(const pink::HttpRequest* req, pink::HttpResponse* res) {
    std::cout << "handle get"<< std::endl;
    std::cout << " + method: " << req->method << std::endl;
    std::cout << " + path: " << req->path << std::endl;
    std::cout << " + version: " << req->version << std::endl;
    std::cout << " + content: " << req->content<< std::endl;
    std::cout << " + headers: " << std::endl;
    for (auto& h : req->headers) {
      std::cout << "   + " << h.first << ":" << h.second << std::endl;
    }
    std::cout << " + query_params: " << std::endl;
    for (auto& q : req->query_params) {
      std::cout << "   + " << q.first << ":" << q.second << std::endl;
    }
    std::cout << " + post_params: " << std::endl;
    for (auto& q : req->post_params) {
      std::cout << "   + " << q.first << ":" << q.second << std::endl;
    }

    res->SetStatusCode(200);
    res->SetBody("china");
  }
};

class MyConnFactory : public ConnFactory {
 public:
  virtual PinkConn* NewPinkConn(int connfd, const std::string& ip_port, Thread* thread) const {
    return new MyHttpConn(connfd, ip_port, thread);
  }
};

int main(int argc, char* argv[]) {
  std::string ip;
  int port;
  if (argc < 3) {
    printf("Usage: ./http_server ip port");
  } else {
    ip.assign(argv[1]);
    port = atoi(argv[2]);
  }

  ConnFactory* my_conn_factory = new MyConnFactory();
  ServerThread *st = NewDispatchThread(port, 4, my_conn_factory, 1000);

  st->StartThread();
  st->JoinThread();

  delete st;
  delete my_conn_factory;

  return 0;
}
