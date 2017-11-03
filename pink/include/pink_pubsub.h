// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_SRC_PUBSUB_H_
#define PINK_SRC_PUBSUB_H_

#include <sys/epoll.h>

#include <string>
#include <functional>
#include <queue>
#include <map>
#include <atomic>
#include <vector>
#include <set>
#include <fcntl.h>

#include "slash/include/xdebug.h"
#include "slash/include/slash_mutex.h"
#include "slash/include/slash_string.h"

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

class PubSubThread : public Thread {
 public:
  explicit PubSubThread();

  virtual ~PubSubThread();

  void set_keepalive_timeout(int timeout) {
    keepalive_timeout_ = timeout;
  }

  int conn_num() const;

  std::vector<ServerThread::ConnInfo> conns_info() const;

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

  void* private_data_;

  // PubSub
  int MessageNum() {
    slash::MutexLock l(&msg_mutex_);
    return msgs_.size(); 
  }

  void RemoveConn(int fd);

  int Publish(int fd, const std::string& channel, const std::string& msg);

  void Subscribe(PinkConn* conn, const std::vector<std::string> channels, bool pattern, std::vector<std::pair<std::string, int>>& result);

  int UnSubscribe(PinkConn* conn, const std::vector<std::string> channels, bool pattern, std::vector<std::pair<std::string, int>>& result);
  
  pink::WriteStatus SendResponse(int32_t fd, const std::string& resp);
    
 private:
  ConnFactory *conn_factory_;
  int cron_interval_;
  
  int msg_pfd_[2];
  bool should_exit_;
  slash::CondVar msg_rsignal_;
  slash::CondVar receiver_rsignal_;
  slash::Mutex msg_mutex_;
  slash::Mutex channel_mutex_;
  slash::Mutex pattern_mutex_;
  slash::Mutex receiver_mutex_;

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
  
  // PubSub
  std::map<std::string, std::vector<PinkConn* >> pubsub_channel_;    // channel <---> fds
  std::map<std::string, std::vector<PinkConn* >> pubsub_pattern_;    // channel <---> fds
  std::map<PinkConn*, std::vector<std::string>> client_channel_;     // client  <---> channels
  std::map<PinkConn*, std::vector<std::string>> client_pattern_;     // client  <---> patterns;
  std::map<int, std::map<std::string, std::string>> msgs_;           // fd      <---> (channel, msg)
  std::map<int, int> receivers_;                                     // fd      <---> receivers
  std::map<int, PinkConn*> conns_;                                   // fd      <---> PinkConn
  // No copying allowed
  PubSubThread(const PubSubThread&);
  void operator=(const PubSubThread&);
};  // class PubSubThread

}  // namespace pink
#endif  // PINK_SRC_PUBSUB_THREAD_H_
