// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_SRC_PINK_EPOLL_H_
#define PINK_SRC_PINK_EPOLL_H_
#include "sys/epoll.h"

namespace pink {

struct PinkFiredEvent {
  int fd;
  int mask;
};

class PinkEpoll {
 public:
  PinkEpoll();
  ~PinkEpoll();
  int PinkAddEvent(const int fd, const int mask);
  int PinkDelEvent(const int fd);
  int PinkModEvent(const int fd, const int old_mask, const int mask);

  int PinkPoll(const int timeout);

  PinkFiredEvent *firedevent() const { return firedevent_; }

 private:
  int epfd_;
  struct epoll_event *events_;
  int timeout_;
  PinkFiredEvent *firedevent_;
};

}  // namespace pink
#endif  // PINK_SRC_PINK_EPOLL_H_
