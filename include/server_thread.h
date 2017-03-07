// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_SERVER_THREAD_H_
#define INCLUDE_SERVER_THREAD_H_

#include <sys/epoll.h>

#include <set>
#include <vector>

#include "slash/include/slash_status.h"
#include "include/pink_define.h"
#include "include/pink_thread.h"
#include "include/pink_mutex.h"

namespace pink {

class ServerSocket;
class PinkEpoll;
struct PinkFiredEvent;
class ConnFactory;
class WorkerThread;

class ServerThread : public Thread {
 public:
  /**
   * @brief
   *
   * @param port the port number
   * @param work_num
   * @param worker_thread the worker thred we define
   * @param cron_interval the cron job interval
   */
  ServerThread(int port, int cron_interval);

  /**
   * @brief
   *
   * @param ip the ip string
   * @param port the port number
   * @param work_num
   * @param worker_thread the worker thred we define
   * @param cron_interval the cron job interval
   */
  ServerThread(const std::string& bind_ip, int port, int cron_interval);

  /**
   * @brief
   *
   * @param ips the ip string set
   * @param port the port number
   * @param work_num
   * @param worker_thread the worker thred we define
   * @param cron_interval the cron job interval
   */
  ServerThread(const std::set<std::string>& bind_ips, int port, int cron_interval);

  virtual ~ServerThread();

  virtual int StartThread() override;


 protected:
  int cron_interval_;

  /*
   * The tcp server port and address
   */
  std::vector<ServerSocket*> server_sockets_;
  std::set<int32_t> server_fds_;

  int port_;
  std::set<std::string> ips_;

  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;

  virtual int InitHandle();
  virtual void CronHandle() {}
  virtual bool AccessHandle(std::string& ip) {
    return true;
  }

  virtual void *ThreadMain() override;

  /*
   * The server connection and event handle
   */
  virtual void HandleNewConn(const int connfd, const std::string& ip_port) = 0;
  virtual void HandleConnEvent(PinkFiredEvent *pfe) = 0;
};

extern ServerThread *NewHolyThread(int port, ConnFactory *conn_factory, int
    cron_interval);
extern ServerThread *NewHolyThread(const std::string &bind_ip, int port,
    ConnFactory *conn_factory, int cron_interval);
extern ServerThread *NewHolyThread(const std::set<std::string>& bind_ips, int
    port, ConnFactory *conn_factory, int cron_interval);

extern ServerThread *NewDispatchThread(
    int port,
    int work_num, Thread **worker_thread,
    int cron_interval = 0);
extern ServerThread *NewDispatchThread(
    const std::string &ip, int port,
    int work_num, Thread **worker_thread,
    int cron_interval = 0);
extern ServerThread *NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, Thread **worker_thread,
    int cron_interval = 0);

} // namespace pink

#endif
