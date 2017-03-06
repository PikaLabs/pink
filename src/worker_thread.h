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

#include <google/protobuf/message.h>

#include "include/pink_thread.h"
#include "include/pink_mutex.h"
#include "include/pink_define.h"
#include "include/xdebug.h"

namespace pink {

class PinkItem;
class PinkEpoll;
class PinkFiredEvent;
class PinkConn;
class ConnFactory;

class WorkerThread : public Thread {
 public:
  explicit WorkerThread(ConnFactory *conn_factory, int cron_interval = 0);
  virtual ~WorkerThread();

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
  Mutex mutex_;

  /*
   *  public for external statistics
   */
  pthread_rwlock_t rwlock_;
  std::map<int, PinkConn *> conns_;


 private:
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

  virtual void *ThreadMain() override;

  // clean conns
  void Cleanup();

};  // class WorkerThread

}  // namespace pink

#endif  // PINK_INCLUDE_WORKER_THREAD_H_
