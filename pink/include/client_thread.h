// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_CLIENT_THREAD_H_
#define PINK_INCLUDE_CLIENT_THREAD_H_

#include <sys/epoll.h>

#include <set>
#include <string>
#include <map>
#include <memory>

#include "slash/include/slash_status.h"
#include "slash/include/slash_mutex.h"
#include "pink/include/pink_thread.h"

// remove 'unused parameter' warning
#define UNUSED(expr) do { (void)(expr); } while (0)

namespace pink {

class PinkEpoll;
struct PinkFiredEvent;
class ConnFactory;
class PinkConn;

class ClientThread : public Thread {
 public:
  ClientThread(ConnFactory* conn_factory, int cron_interval, int keepalive_timeout);
  virtual ~ClientThread();
  /*
   * StartThread will return the error code as pthread_create return
   *  Return 0 if success
   */
  virtual int StartThread() override;
  void Write(const std::string& ip, const int port, const std::string& msg);

 private:
  virtual void *ThreadMain() override;

  void InternalDebugPrint();
  // Set connect fd into epoll
  // connect condition: no EPOLLERR EPOLLHUP events,  no error in socket opt
  void ProcessConnectStatus(PinkFiredEvent* pfe, int* should_close);
  void SetWaitConnectOnEpoll(int sockfd);

  void NewConnection(const std::string& peer_ip, int peer_port, int sockfd);
  // Try to connect fd noblock, if return EINPROGRESS or EAGAIN or EWOULDBLOCK
  // put this fd in epoll (SetWaitConnectOnEpoll), process in ProcessConnectStatus
  slash::Status ScheduleConnect(const std::string& dst_ip, int dst_port);
  void CloseFd(std::shared_ptr<PinkConn> conn);
  void DoCronTask();
  void NotifyWrite(const std::string ip_port);
  void ProcessNotifyEvents(const PinkFiredEvent* pfe);

  int keepalive_timeout_;
  int cron_interval_;

  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;

  ConnFactory *conn_factory_;

  slash::Mutex mu_;
  std::map<std::string, std::string> to_send_;  // ip+:+port, to_send_msg

  std::map<int, std::shared_ptr<PinkConn>> fd_conns_;
  std::map<std::string, std::shared_ptr<PinkConn>> ipport_conns_;
  std::set<int> connecting_fds_;
};

}  // namespace pink
#endif  // PINK_INCLUDE_CLIENT_THREAD_H_
