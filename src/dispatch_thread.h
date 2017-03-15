// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_DISPATCH_THREAD_H_
#define INCLUDE_DISPATCH_THREAD_H_

#include <set>
#include <queue>
#include <string>

#include "include/xdebug.h"
#include "include/server_thread.h"

namespace pink {

class PinkItem;
class PinkFiredEvent;
class WorkerThread;

class DispatchThread : public ServerThread {
 public:
  // This type Dispatch thread just get Connection and then Dispatch the fd to
  // worker thead
  DispatchThread(int port,
                 int work_num, Thread **worker_thread,
                 int cron_interval = 0);
  DispatchThread(const std::string &ip, int port,
                 int work_num, Thread **worker_thread,
                 int cron_interval = 0);
  DispatchThread(const std::set<std::string>& ips, int port,
                 int work_num, Thread **worker_thread,
                 int cron_interval = 0);

  virtual ~DispatchThread() {
  }

  int work_num() {
    return work_num_;
  }

  WorkerThread** worker_thread() {
    return worker_thread_;
  }

 private:
  /*
   * Here we used auto poll to find the next work thread,
   * last_thread_ is the last work thread
   */
  int last_thread_;
  int work_num_;
  /*
   * This is the work threads
   */
  WorkerThread **worker_thread_;

  void HandleNewConn(const int connfd, const std::string& ip_port) override;
  void HandleConnEvent(PinkFiredEvent *pfe) override {}

  void StartWorkerThreads();

  // No copying allowed
  DispatchThread(const DispatchThread&);
  void operator=(const DispatchThread&);
};  // class DispatchThread


}  // namespace pink

#endif  // INCLUDE_DISPATCH_THREAD_H_
