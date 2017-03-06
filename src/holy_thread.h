// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_HOLY_THREAD_H_
#define INCLUDE_HOLY_THREAD_H_

#include <map>
#include <set>
#include <string>

#include "include/xdebug.h"
#include "include/server_thread.h"
#include "include/pink_conn.h"
#include "include/pink_mutex.h"

namespace pink {
class PinkConn;

class HolyThread: public ServerThread {
 public:
  // This type thread thread will listen and work self list redis thread
  HolyThread(int port, ConnFactory *conn_factory, int cron_interval = 0);
  HolyThread(const std::string& bind_ip, int port, ConnFactory *conn_factory, int cron_interval = 0);
  HolyThread(const std::set<std::string>& bind_ips, int port, ConnFactory *conn_factory, int cron_interval = 0);
  virtual ~HolyThread();

  /*
   *  public for external statistics
   */
  pthread_rwlock_t rwlock_;
  std::map<int, PinkConn *> conns_;

 private:
  void CronHandle() override {}
  bool AccessHandle(std::string& ip) override {
    return true;
  }

  ConnFactory *conn_factory_;
  void HandleNewConn(const int connfd, const std::string &ip_port) override;
  void HandleConnEvent(PinkFiredEvent *pfe) override;

  void Cleanup();
};  // class HolyThread


}  // namespace pink

#endif  // INCLUDE_HOLY_THREAD_H_
