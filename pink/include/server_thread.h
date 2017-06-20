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
#include "slash/include/slash_mutex.h"
#include "pink/include/pink_define.h"
#include "pink/include/pink_thread.h"

namespace pink {

class ServerSocket;
class PinkEpoll;
class PinkConn;
struct PinkFiredEvent;
class ConnFactory;
class WorkerThread;

class ServerHandle {
 public:
  ServerHandle() {}
  virtual ~ServerHandle() {}

  virtual void CronHandle() const {};
  virtual bool AccessHandle(std::string& ip) const { return true; };
  virtual int CreateWorkerSpecificData(void** data) const { return 0; }
  virtual int DeleteWorkerSpecificData(void* data) const { return 0; }
};

const int kDefaultKeepAliveTime = 60; // (s)

class ServerThread : public Thread {
 public:
  /*
   * StartThread will return the error code as pthread_create
   */
  virtual int StartThread() override;

  virtual void set_keepalive_timeout(int timeout) = 0;

  virtual int conn_num() = 0; 

  virtual void KillAllConns() = 0;
  virtual void KillConn(const std::string& ip_port) = 0;

  virtual ~ServerThread();

 protected:
  ServerThread(int port, int cron_interval, const ServerHandle *handle);
  ServerThread(const std::string& bind_ip, int port, int cron_interval,
               const ServerHandle *handle);
  ServerThread(const std::set<std::string>& bind_ips, int port,
               int cron_interval, const ServerHandle *handle);

  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;

 private:
  friend class DispatchThread;
  friend class HolyThread;

  int cron_interval_;
  virtual void DoCronTask();

  const ServerHandle *handle_;
  bool own_handle_;
  /*
   * The tcp server port and address
   */
  int port_;
  std::set<std::string> ips_;
  std::vector<ServerSocket*> server_sockets_;
  std::set<int32_t> server_fds_;

  virtual int InitHandle();
  virtual void *ThreadMain() override;
  /*
   * The server connection and event handle
   */
  virtual void HandleNewConn(int connfd, const std::string& ip_port) = 0;
  virtual void HandleConnEvent(PinkFiredEvent *pfe) = 0;

};

extern ServerThread *NewHolyThread(
    int port,
    ConnFactory *conn_factory,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr);
extern ServerThread *NewHolyThread(
    const std::string &bind_ip, int port,
    ConnFactory *conn_factory,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr);
extern ServerThread *NewHolyThread(
    const std::set<std::string>& bind_ips, int port,
    ConnFactory *conn_factory,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr);

/**
 * This type Dispatch thread just get Connection and then Dispatch the fd to
 * worker thread
 *
 * @brief
 *
 * @param port          the port number
 * @param conn_factory  connection factory object
 * @param cron_interval the cron job interval
 * @param queue_limit   the size limit of workers' connection queue
 * @param handle        the server's handle (e.g. CronHandle, AccessHandle...)
 * @param ehandle       the worker's enviroment setting handle
 */
extern ServerThread *NewDispatchThread(
    int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval = 0,
    int queue_limit = 1000,
    const ServerHandle* handle = nullptr);
extern ServerThread *NewDispatchThread(
    const std::string &ip, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval = 0,
    int queue_limit = 1000,
    const ServerHandle* handle = nullptr);
extern ServerThread *NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval = 0,
    int queue_limit = 1000,
    const ServerHandle* handle = nullptr);

} // namespace pink

#endif
