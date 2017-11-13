// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_SRC_WORKER_THREAD_H_
#define PINK_SRC_WORKER_THREAD_H_

#include <sys/epoll.h>

#include <string>
#include <functional>
#include <queue>
#include <map>
#include <atomic>
#include <vector>
#include <set>

#include "slash/include/xdebug.h"
#include "slash/include/slash_mutex.h"

#include "pink/include/server_thread.h"
#include "pink/src/pink_epoll.h"
#include "pink/include/pink_thread.h"
#include "pink/include/pink_define.h"

namespace pink {

class PinkItem;
class PinkEpoll;
class PinkFiredEvent;
class PinkConn;
class ConnFactory;

class WorkerThread : public Thread {
 public:
  explicit WorkerThread(ConnFactory *conn_factory, ServerThread* server_thread,
                        int cron_interval = 0);

  virtual ~WorkerThread();

  void set_keepalive_timeout(int timeout) {
    keepalive_timeout_ = timeout;
  }

  int conn_num() const;

  std::vector<ServerThread::ConnInfo> conns_info() const;

  PinkConn* MoveConnOut(int fd);

  /*
   * The PbItem queue is the fd queue, receive from dispatch thread
   */
  std::queue<PinkItem> conn_queue_;

  int notify_receive_fd() {
    return notify_receive_fd_;
  }
  int notify_send_fd() {
    return notify_send_fd_;
  }
  PinkEpoll* pink_epoll() {
    return pink_epoll_;
  }
  bool TryKillConn(const std::string& ip_port);

  slash::Mutex mutex_;

  mutable slash::RWMutex rwlock_; /* For external statistics */
  std::map<int, PinkConn*> conns_;

  void* private_data_;

 private:
  ServerThread* server_thread_;
  ConnFactory *conn_factory_;
  int cron_interval_;

  /*
   * These two fd receive the notify from dispatch thread
   */
  int notify_receive_fd_;
  int notify_send_fd_;

  /*
   * The epoll handler
   */
  PinkEpoll *pink_epoll_;

  std::atomic<int> keepalive_timeout_;  // keepalive second

  virtual void *ThreadMain() override;
  void DoCronTask();

  slash::Mutex killer_mutex_;
  std::set<std::string> deleting_conn_ipport_;

  // clean conns
  void CloseFd(PinkConn* conn);
  void Cleanup();
};  // class WorkerThread

}  // namespace pink
#endif  // PINK_SRC_WORKER_THREAD_H_
