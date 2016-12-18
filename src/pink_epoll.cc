// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "src/pink_epoll.h"

#include <linux/version.h>
#include <fcntl.h>

#include "include/pink_define.h"
#include "include/xdebug.h"

namespace pink {

PinkEpoll::PinkEpoll() : timeout_(1000) {
#if defined(EPOLL_CLOEXEC)
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
#else
    epfd_ = epoll_create(1024);
#endif

  fcntl(epfd_, F_SETFD, fcntl(epfd_, F_GETFD) | FD_CLOEXEC);

  if (epfd_ < 0) {
    log_err("epoll create fail");
    exit(1);
  }
  events_ = (struct epoll_event *)malloc(sizeof(struct epoll_event) * PINK_MAX_CLIENTS);

  firedevent_ = (PinkFiredEvent *)malloc(sizeof(PinkFiredEvent) * PINK_MAX_CLIENTS);
}

PinkEpoll::~PinkEpoll() {
  free(firedevent_);
  free(events_);
  close(epfd_);
}

int PinkEpoll::PinkAddEvent(int fd, int mask) {
  struct epoll_event ee;
  ee.data.fd = fd;
  ee.events = mask;
  if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ee) == -1) {
    return -1;
  }
  return 0;
}

int PinkEpoll::PinkModEvent(int fd, int oMask, int mask) {
  struct epoll_event ee;
  ee.data.fd = fd;
  ee.events = (oMask | mask);
  if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ee) == -1) {
    return -1;
  }
  return 0;
}

void PinkEpoll::PinkDelEvent(int fd) {
  /*
   * Kernel < 2.6.9 need a non null event point to EPOLL_CTL_DEL
   */
  struct epoll_event ee;
  ee.data.fd = fd;
  epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &ee);
}

int PinkEpoll::PinkPoll(int timeout) {
  int retval, numevents = 0;
  retval = epoll_wait(epfd_, events_, PINK_MAX_CLIENTS, timeout);
  if (retval > 0) {
    numevents = retval;
    for (int i = 0; i < numevents; i++) {
      int mask = 0;
      firedevent_[i].fd = (events_ + i)->data.fd;
      /*
       * log_info("events + i events %d", (events_ + i)->events);
       */
      if ((events_ + i)->events & EPOLLIN) {
        mask |= EPOLLIN;
      }
      if ((events_ + i)->events & EPOLLOUT) {
        mask |= EPOLLOUT;
      }
      if ((events_ + i)->events & EPOLLERR) {
        mask |= EPOLLERR;
      }
      if ((events_ + i)->events & EPOLLHUP) {
        mask |= EPOLLHUP;
      }
      firedevent_[i].mask = mask;
    }
  }
  return numevents;
}

}  // namespace pink
