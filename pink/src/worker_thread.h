// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_WORKER_THREAD_H_
#define PINK_INCLUDE_WORKER_THREAD_H_

#include <sys/epoll.h>

#include <string>
#include <functional>
#include <queue>
#include <map>
#include <atomic>

#include <google/protobuf/message.h>

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

  int conn_num() {
    slash::ReadLock l(&rwlock_);
    return conns_.size();
  } 


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
  slash::Mutex mutex_;

  slash::RWMutex rwlock_;
  std::map<int, PinkConn*> conns_;

 private:
  friend class DispatchThread;

  ServerThread* server_thread_;
  ConnFactory *conn_factory_;
  int cron_interval_;

  void* private_data_;

  /*
   * These two fd receive the notify from dispatch thread
   */
  int notify_receive_fd_;
  int notify_send_fd_;

  /*
   * The epoll handler
   */
  PinkEpoll *pink_epoll_;

  std::atomic<int> keepalive_timeout_; // keepalive second

  virtual void *ThreadMain() override;
  void DoCronTask();

  // clean conns
  void KillAllConns();
  void KillConn(const std::string& ip_port);
};  // class WorkerThread

}  // namespace pink

#endif  // PINK_INCLUDE_WORKER_THREAD_H_
