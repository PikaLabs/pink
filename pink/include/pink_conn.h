// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_PINK_CONN_H_
#define PINK_INCLUDE_PINK_CONN_H_

#include <sys/time.h>
#include <string>

#ifdef __ENABLE_SSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#include "pink/include/pink_define.h"
#include "pink/include/server_thread.h"

namespace pink {

class Thread;

class PinkConn {
 public:
  PinkConn(const int fd, const std::string &ip_port, ServerThread *thread);
  virtual ~PinkConn();

  /*
   * Set the fd to nonblock && set the flag_ the the fd flag
   */
  bool SetNonblock();

#ifdef __ENABLE_SSL
  bool CreateSSL(SSL_CTX* ssl_ctx);
#endif

  virtual ReadStatus GetRequest() = 0;
  virtual WriteStatus SendReply() = 0;

  int flags() const {
    return flags_;
  }

  void set_fd(const int fd) {
    fd_ = fd;
  }

  int fd() const {
    return fd_;
  }

  std::string ip_port() const {
    return ip_port_;
  }

  void set_is_reply(const bool is_reply) {
    is_reply_ = is_reply;
  }

  bool is_reply() const {
    return is_reply_;
  }

  void set_last_interaction(const struct timeval &now) {
    last_interaction_ = now;
  }

  struct timeval last_interaction() const {
    return last_interaction_;
  }

  ServerThread *server_thread() const {
    return server_thread_;
  }

#ifdef __ENABLE_SSL
  SSL* ssl() {
    return ssl_;
  }

  bool security() {
    return ssl_ != nullptr;
  }
#endif

 private:
  int fd_;
  std::string ip_port_;
  bool is_reply_;
  struct timeval last_interaction_;
  int flags_;

#ifdef __ENABLE_SSL
  SSL* ssl_;
#endif

  // the server thread this conn belong to
  ServerThread *server_thread_;

  /*
   * No allowed copy and copy assign operator
   */
  PinkConn(const PinkConn&);
  void operator=(const PinkConn&);
};


/*
 * for every conn, we need create a corresponding ConnFactory
 */
class ConnFactory {
 public:
  virtual ~ConnFactory() {}
  virtual PinkConn* NewPinkConn(
    int connfd,
    const std::string &ip_port,
    ServerThread *server_thread,
    void* worker_private_data /* Has set in ThreadEnvHandle */) const = 0;
};

}  // namespace pink

#endif  // PINK_INCLUDE_PINK_CONN_H_
