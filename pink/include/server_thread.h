// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_SERVER_THREAD_H_
#define INCLUDE_SERVER_THREAD_H_

#include <sys/epoll.h>

#include <set>
#include <vector>
#include <memory>

#ifdef __ENABLE_SSL
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#endif

#include "slash/include/slash_status.h"
#include "slash/include/slash_mutex.h"
#include "pink/include/pink_define.h"
#include "pink/include/pink_thread.h"

// remove 'unused parameter' warning
#define UNUSED(expr) do { (void)(expr); } while (0)

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

  virtual void CronHandle() const {}
  virtual void FdTimeoutHandle(int fd, const std::string& ip_port) const {
    UNUSED(fd);
    UNUSED(ip_port);
  }
  virtual void FdClosedHandle(int fd, const std::string& ip_port) const {
    UNUSED(fd);
    UNUSED(ip_port);
  }
  virtual bool AccessHandle(std::string& ip) const {
    UNUSED(ip);
    return true;
  }
  virtual int CreateWorkerSpecificData(void** data) const {
    UNUSED(data);
    return 0;
  }
  virtual int DeleteWorkerSpecificData(void* data) const {
    UNUSED(data);
    return 0;
  }
};

const std::string kKillAllConnsTask = "kill_all_conns";

const int kDefaultKeepAliveTime = 60; // (s)

class ServerThread : public Thread {
 public:
  ServerThread(int port, int cron_interval, const ServerHandle *handle);
  ServerThread(const std::string& bind_ip, int port, int cron_interval,
               const ServerHandle *handle);
  ServerThread(const std::set<std::string>& bind_ips, int port,
               int cron_interval, const ServerHandle *handle);

#ifdef __ENABLE_SSL
  /*
   * Enable TLS, set before StartThread, default: false
   * Just HTTPConn has supported for now.
   */
  int EnableSecurity(const std::string& cert_file, const std::string& key_file);
  SSL_CTX* ssl_ctx() { return ssl_ctx_; }
  bool security() { return security_; }
#endif

  /*
   * StartThread will return the error code as pthread_create
   */
  virtual int StartThread() override;

  virtual void set_keepalive_timeout(int timeout) = 0;

  virtual int conn_num() const = 0;

  struct ConnInfo {
    int fd;
    std::string ip_port;
    struct timeval last_interaction;
  };
  virtual std::vector<ConnInfo> conns_info() const = 0;

  // Move out from server thread
  virtual PinkConn* MoveConnOut(int fd) = 0;

  virtual void KillAllConns() = 0;
  virtual bool KillConn(const std::string& ip_port) = 0;

  virtual ~ServerThread();

 protected:
  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;

 private:
  friend class HolyThread;
  friend class DispatchThread;
  friend class WorkerThread;

  int cron_interval_;
  virtual void DoCronTask();

  const ServerHandle *handle_;
  bool own_handle_;

#ifdef __ENABLE_SSL
  bool security_;
  SSL_CTX* ssl_ctx_;
#endif

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
    int cron_interval = 0, int queue_limit = 1000,
    const ServerHandle* handle = nullptr);
extern ServerThread *NewDispatchThread(
    const std::string &ip, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval = 0, int queue_limit = 1000,
    const ServerHandle* handle = nullptr);
extern ServerThread *NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval = 0, int queue_limit = 1000,
    const ServerHandle* handle = nullptr);

} // namespace pink

#endif
