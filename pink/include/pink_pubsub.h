// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_PUBSUB_H_
#define PINK_INCLUDE_PUBSUB_H_

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

class PinkEpoll;
class PinkFiredEvent;
class PinkConn;

class PubSubThread : public Thread {
 public:
  explicit PubSubThread();

  virtual ~PubSubThread();

  PinkEpoll* pink_epoll() {
    return pink_epoll_;
  }

  // PubSub

  int Publish(int fd, const std::string& channel, const std::string& msg);

  void Subscribe(PinkConn* conn, const std::vector<std::string> channels, bool pattern, std::vector<std::pair<std::string, int>>& result);

  int UnSubscribe(PinkConn* conn, const std::vector<std::string> channels, bool pattern, std::vector<std::pair<std::string, int>>& result);

  void PubSub(std::map<std::string, std::vector<PinkConn* >>& pubsub_channel, std::map<std::string, std::vector<PinkConn* >>& pubsub_pattern);
  
 private:
  void RemoveConn(PinkConn* conn);
  int msg_pfd_[2];
  bool should_exit_;
  
  slash::Mutex channel_mutex_;
  slash::Mutex pattern_mutex_;
  mutable slash::RWMutex rwlock_; /* For external statistics */

  slash::Mutex pub_mutex_;

  slash::CondVar receiver_rsignal_;
  slash::Mutex receiver_mutex_;

  std::string channel_;
  std::string message_;
  int receivers_ = -1;

  /*
   * The epoll handler
   */
  PinkEpoll *pink_epoll_;

  virtual void *ThreadMain() override;

  // clean conns
  void CloseFd(PinkConn* conn);
  void Cleanup();
  
  // PubSub
  std::map<std::string, std::vector<PinkConn* >> pubsub_channel_;    // channel <---> fds
  std::map<std::string, std::vector<PinkConn* >> pubsub_pattern_;    // channel <---> fds
  std::map<PinkConn*, std::vector<std::string>> client_channel_;     // client  <---> channels
  std::map<PinkConn*, std::vector<std::string>> client_pattern_;     // client  <---> patterns;
  std::map<int, PinkConn*> conns_;                                   // fd      <---> PinkConn
  
  // No copying allowed
  PubSubThread(const PubSubThread&);
  void operator=(const PubSubThread&);

};  // class PubSubThread

}  // namespace pink
#endif  // PINK_INCLUDE_PUBSUB_H_
