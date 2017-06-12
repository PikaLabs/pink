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
struct PinkFiredEvent;
class ConnFactory;
class WorkerThread;

class ServerHandle {
 public:
  ServerHandle() {}
  virtual ~ServerHandle() {}

  virtual void CronHandle() const {};
  virtual bool AccessHandle(std::string& ip) const {
    return true;
  };
};

const int kDefaultKeepAliveTime = 60; // (s)

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
  ServerThread(int port, int cron_interval = 0,
               const ServerHandle *handle = nullptr);

  /**
   * @brief
   *
   * @param ip the ip string
   * @param port the port number
   * @param work_num
   * @param worker_thread the worker thred we define
   * @param cron_interval the cron job interval
   */
  ServerThread(const std::string& bind_ip, int port, int cron_interval = 0,
               const ServerHandle *handle = nullptr);

  /**
   * @brief
   *
   * @param ips the ip string set
   * @param port the port number
   * @param work_num
   * @param worker_thread the worker thred we define
   * @param cron_interval the cron job interval
   */
  ServerThread(const std::set<std::string>& bind_ips, int port,
               int cron_interval = 0, const ServerHandle *handle = nullptr);

  virtual ~ServerThread();

  /*
   * StartThread will return the error code as pthread_create
   */
  virtual int StartThread() override;

  virtual void set_keepalive_timeout(int timeout) = 0;

  virtual int conn_num() = 0; 

 protected:
  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;

 private:
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
    const ServerHandle* handle = nullptr,
    const ThreadEnvHandle* thandle = nullptr);
extern ServerThread *NewHolyThread(
    const std::string &bind_ip, int port,
    ConnFactory *conn_factory,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr,
    const ThreadEnvHandle* thandle = nullptr);
extern ServerThread *NewHolyThread(
    const std::set<std::string>& bind_ips, int port,
    ConnFactory *conn_factory,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr,
    const ThreadEnvHandle* thandle = nullptr);

extern ServerThread *NewDispatchThread(
    int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr,
    const ThreadEnvHandle* thandle = nullptr);
extern ServerThread *NewDispatchThread(
    const std::string &ip, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr,
    const ThreadEnvHandle* thandle = nullptr);
extern ServerThread *NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr,
    const ThreadEnvHandle* thandle = nullptr);

extern ServerThread* NewDispatchThread(
    int port,
    int work_num, Thread **worker_thread,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr);
extern ServerThread* NewDispatchThread(
    const std::string &ip, int port,
    int work_num, Thread **worker_thread,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr);
extern ServerThread* NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, Thread **worker_thread,
    int cron_interval = 0,
    const ServerHandle* handle = nullptr);

extern Thread *NewWorkerThread(
    ConnFactory *conn_factory,
    int cron_interval = 0);

} // namespace pink

#endif
