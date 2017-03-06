// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#ifndef SRC_REDIS_CONN_FACTORY_H_
#define SRC_REDIS_CONN_FACTORY_H_

#include "include/pink_conn.h"

class RedisConn;
class RedisConnFactory : public ConnFactory {
 public:
  virtual PinkConn* NewPinkConn(int connfd, const std::string &ip_port, Thread *thread) {;
    return new RedisConn(connfd, ip_port, thread);
  }
}

extern ConnFactory *NewPbConnFactory(int connfd, const std::string &ip_port, Thread *thread) {
  return new RedisConnFactory(connfd, ip_port, thread);
}

#endif // SRC_REDIS_CONN_FACTORY_H_
