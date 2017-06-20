// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_STANDARD_THREAD_H_
#define INCLUDE_STANDARD_THREAD_H_

#include <set>
#include <queue>
#include <string>

#include "slash/include/xdebug.h"
#include "pink/include/server_thread.h"
#include "pink/include/pink_conn.h"

namespace pink {

class PinkItem;
class PinkFiredEvent;
class WorkerThread;

class StandardServer : public ServerThread {
 public:
   StandardServer(int port,
                 int work_num, ConnFactory* conn_factory,
                 int queue_limit,
                 int cron_interval,
                 const ServerHandle* handle);
  StandardServer(const std::string &ip, int port,
                 int work_num, ConnFactory* conn_factory,
                 int queue_limit,
                 int cron_interval,
                 const ServerHandle* handle);
  StandardServer(const std::set<std::string>& ips, int port,
                 int work_num, ConnFactory* conn_factory,
                 int queue_limit,
                 int cron_interval,
                 const ServerHandle* handle);

  virtual ~StandardServer();

  virtual int StartThread() override;

  virtual int StopThread() override;
  
  virtual void set_keepalive_timeout(int timeout) override;

  virtual int conn_num() override; 

	virtual void KillAllConns() override;

	virtual void KillConn(const std::string& ip_port) override;

 private:

  /*
   * case 1: work_num_ > 1;
   * In this case, StandardServer includes at least 2 threads:
   * one is Dispatcher, the others is Workers
   *
   *
   * Here we used auto poll to find the next work thread,
   * last_thread_ is the last work thread
   */
  int last_thread_;
  int work_num_;
  /*
   * This is the work threads
   */
  WorkerThread** worker_thread_;
  int queue_limit_;
  // case 1 end 


  /*
   * case 2: work_num_ = 0
   * In this case, StandardServer includes only 1 thread:
   * Dispatcher + Workers is in 1 thread
   *
   *
   *  For external statistics
   */
  slash::RWMutex rwlock_;
  std::map<int, PinkConn*> conns_;

  ConnFactory *conn_factory_;
  void* private_data_;

  std::atomic<int> keepalive_timeout_; // keepalive second

  slash::Mutex killer_mutex_;
  std::set<std::string> deleting_conn_ipport_;
  void Cleanup();
  // case 2 end


  void DoCronTask() override;

  void HandleNewConn(const int connfd, const std::string& ip_port) override;
  void HandleConnEvent(PinkFiredEvent *pfe) override; 

  // No copying allowed
  StandardServer(const StandardServer&);
  void operator=(const StandardServer&);
};  // class StandardServer


}  // namespace pink

#endif  // INCLUDE_STANDARD_THREAD_H_
