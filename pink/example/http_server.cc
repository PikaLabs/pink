// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <string>
#include <chrono>
#include <atomic>
#include <signal.h>

#include "slash/include/slash_status.h"
#include "pink/include/pink_thread.h"
#include "pink/src/worker_thread.h"
#include "pink/src/dispatch_thread.h"
#include "pink/include/http_conn.h"

using namespace pink;

class MyHttpConn : public pink::HttpConn {
 public:
  class MyHttpHandle : public pink::HttpConn::HttpHandles {
   public:
    std::string resp_body;
    std::chrono::system_clock::time_point start, end;
    std::chrono::duration<double, std::milli> diff;
    bool need_100_continue;

    // Request handles
    virtual bool ReqHeadersHandle(HttpRequest* req) {
      start = std::chrono::system_clock::now();
      if (req->headers.count("expect")) {
        need_100_continue = true;
        return true;
      }
      return false;
    }
    virtual void ReqBodyPartHandle(const char* data, size_t max_size) {
      // std::cout << "ReqBodyPartHandle: " << max_size << std::endl;
      // resp_body.assign(data, max_size);
      // std::cout << resp_body << std::endl;
    }
    virtual void ReqCompleteHandle(HttpRequest* req, HttpResponse* resp) {
    }
    virtual void ConnClosedHandle() {}

    // Response handles
    virtual void RespHeaderHandle(HttpResponse* resp) {
      if (need_100_continue) {
        resp->SetStatusCode(100);
        resp->SetHeaders("Content-Length", 0);
        need_100_continue = false;
        return;
      }
      resp->SetStatusCode(200);
      // resp->SetHeaders("Content-Length", resp_body.size());
      resp->SetHeaders("Content-Length", 0);
      end = std::chrono::system_clock::now();
      diff = end - start;
      std::cout << "Use: " << diff.count() << " ms" << std::endl;
    }
    virtual int RespBodyPartHandle(char* buf, size_t max_size) {
      // memcpy(buf, resp_body.data(), std::min(max_size, resp_body.size()));;
      return 0;
    }
  };

  MyHttpConn(const int fd, const std::string& ip_port, Thread* worker) :
    HttpConn(fd, ip_port, worker) {
    handles_ = new MyHttpHandle();
  }
  // virtual void DealMessage(const pink::HttpRequest* req, pink::HttpResponse* res) {
  //   std::cout << "handle get"<< std::endl;
  //   std::cout << " + method: " << req->method << std::endl;
  //   std::cout << " + path: " << req->path << std::endl;
  //   std::cout << " + version: " << req->version << std::endl;
  //   std::cout << " + content: " << req->content<< std::endl;
  //   std::cout << " + headers: " << std::endl;
  //   for (auto& h : req->headers) {
  //     std::cout << "   + " << h.first << ":" << h.second << std::endl;
  //   }
  //   std::cout << " + query_params: " << std::endl;
  //   for (auto& q : req->query_params) {
  //     std::cout << "   + " << q.first << ":" << q.second << std::endl;
  //   }
  //   std::cout << " + post_params: " << std::endl;
  //   for (auto& q : req->post_params) {
  //     std::cout << "   + " << q.first << ":" << q.second << std::endl;
  //   }

  //   res->SetStatusCode(200);
  //   res->SetBody("china");
  // }
};

class MyConnFactory : public ConnFactory {
 public:
  virtual PinkConn* NewPinkConn(int connfd, const std::string& ip_port, Thread* thread) const {
    return new MyHttpConn(connfd, ip_port, thread);
  }
};

static std::atomic<bool> running(false);

static void IntSigHandle(const int sig) {
  printf("Catch Signal %d, cleanup...\n", sig);
  running.store(false);
  printf("server Exit");
}

static void SignalSetup() {
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, &IntSigHandle);
  signal(SIGQUIT, &IntSigHandle);
  signal(SIGTERM, &IntSigHandle);
}

int main(int argc, char* argv[]) {
  std::string ip;
  int port;
  if (argc < 3) {
    printf("Usage: ./http_server ip port");
  } else {
    ip.assign(argv[1]);
    port = atoi(argv[2]);
  }

  SignalSetup();

  ConnFactory* my_conn_factory = new MyConnFactory();
  ServerThread *st = NewDispatchThread(port, 4, my_conn_factory, 1000);

  if (st->StartThread() != 0) {
    printf("StartThread error happened!\n");
    exit(-1);
  }
  running.store(true);
  while (running.load()) {
    sleep(1);
  }
  st->StopThread();

  delete st;
  delete my_conn_factory;

  return 0;
}
