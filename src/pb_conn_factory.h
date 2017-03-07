// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#ifndef SRC_PB_CONN_FACTORY_H_
#define SRC_PB_CONN_FACTORY_H_

#include "include/pink_conn.h"
#include "include/pink_thread.h"

class PbConn;
class PbConnFactory : public ConnFactory {
 public:
  virtual PinkConn* NewPinkConn(int connfd, const std::string &ip_port, Thread *thread) {;
    return new PbConn(connfd, ip_port, thread);
  }
}

extern ConnFactory *NewRedisConnFactory() {
  return new PbConnFactory();
}

#endif // SRC_PB_CONN_FACTORY_H_
