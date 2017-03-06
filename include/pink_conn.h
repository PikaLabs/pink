// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#ifndef INCLUDE_PINK_CONN_H_
#define INCLUDE_PINK_CONN_H_

#include <sys/time.h>
#include <string>

#include "include/pink_define.h"

namespace pink {

class Thread;

class PinkConn {
public:
  PinkConn(const int fd, const std::string &ip_port);
  virtual ~PinkConn();
  
  /*
   * Set the fd to nonblock && set the flag_ the the fd flag
   */
  bool SetNonblock();

  virtual ReadStatus GetRequest() = 0;
  virtual WriteStatus SendReply() = 0;

  int flags() const { 
    return flags_; 
  };
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
  };

private:
  
  int fd_;
  std::string ip_port_;
  bool is_reply_;
  struct timeval last_interaction_;
  int flags_;

  /*
   * No allowed copy and copy assign operator
   */
  PinkConn(const PinkConn&);
  void operator=(const PinkConn&);
};

class ConnFactory {
 public:
  virtual ~ConnFactory() {};
  virtual PinkConn* NewPinkConn(int connfd, const std::string &ip_port, Thread *pink_thread) const = 0;
};

extern ConnFactory *NewRedisConnFactory(int connfd, const std::string &ip_port, Thread *thread);
extern ConnFactory *NewPbConnFactory();

}

#endif   // INCLUDE_PINK_CONN_H_
