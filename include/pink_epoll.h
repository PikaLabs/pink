// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#ifndef PINK_EPOLL_H_
#define PINK_EPOLL_H_
#include "sys/epoll.h"
#include "status.h"

namespace pink {

struct PinkFiredEvent {
  int fd_;
  int mask_;
};

class PinkEpoll
{

public:
  PinkEpoll();
  ~PinkEpoll();
  Status PinkAddEvent(int fd, int mask);
  void PinkDelEvent(int fd);
  Status PinkModEvent(int fd, int oMask, int mask);

  int PinkPoll(int timeout);

  PinkFiredEvent *firedevent() { return firedevent_; }

private:

  int epfd_;
  struct epoll_event *events_;
  int timeout_;
  PinkFiredEvent *firedevent_;
};
}

#endif
