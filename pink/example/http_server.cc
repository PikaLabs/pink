// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <string>
#include <chrono>
#include <atomic>
#include <signal.h>

#include "slash/include/slash_status.h"
#include "slash/include/slash_hash.h"
#include "pink/include/pink_thread.h"
#include "pink/src/worker_thread.h"
#include "pink/src/dispatch_thread.h"
#include "pink/include/http_conn.h"

using namespace pink;

class MyHttpHandle : public pink::HttpHandles {
 public:
  std::string body_data;
  std::string zero_space;
  size_t write_pos = 0;
  std::chrono::system_clock::time_point start, end;
  std::chrono::duration<double, std::milli> diff;
  bool need_100_continue;

  // Request handles
  virtual bool ReqHeadersHandle(const HttpRequest* req) {
    req->Dump();
    body_data.clear();

    start = std::chrono::system_clock::now();
    if (req->headers_.count("expect")) {
      need_100_continue = true;
      return true;
    }

    return false;
  }
  virtual void ReqBodyPartHandle(const char* data, size_t size) {
    std::cout << "ReqBodyPartHandle: " << size << std::endl;
    body_data.append(data, size);
  }

  // Response handles
  virtual void RespHeaderHandle(HttpResponse* resp) {
    if (need_100_continue) {
      resp->SetStatusCode(100);
      resp->SetContentLength(0);
      need_100_continue = false;
      return;
    }
    
    std::cout << slash::md5(body_data) << std::endl;

    resp->SetStatusCode(200);
    resp->SetContentLength(body_data.size());
    write_pos = 0;
    end = std::chrono::system_clock::now();
    diff = end - start;
    std::cout << "Use: " << diff.count() << " ms" << std::endl;
  }

  virtual int RespBodyPartHandle(char* buf, size_t max_size) {
    if (need_100_continue) {
      return 0;
    }

    size_t size = std::min(max_size, body_data.size() - write_pos);
    memcpy(buf, body_data.data() + write_pos, size);
    write_pos += size;
    return size;
  }
};

MyHttpHandle my_handles;

class MyConnFactory : public ConnFactory {
 public:
  virtual PinkConn* NewPinkConn(int connfd, const std::string& ip_port, Thread* thread) const {
    return new pink::HttpConn(connfd, ip_port, thread, &my_handles);
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
  int port;
  if (argc < 2) {
    printf("Usage: ./http_server port");
  } else {
    port = atoi(argv[1]);
  }

  SignalSetup();

  ConnFactory* my_conn_factory = new MyConnFactory();
  ServerThread *st = NewDispatchThread(port, 1, my_conn_factory, 1000);

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
