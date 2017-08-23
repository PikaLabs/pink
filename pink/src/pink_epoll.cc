// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "pink/src/pink_epoll.h"

#include <linux/version.h>
#include <fcntl.h>

#include "pink/include/pink_define.h"
#include "slash/include/xdebug.h"

namespace pink {

static const int kPinkMaxClients = 10240;

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
  events_ = (struct epoll_event *)malloc(
      sizeof(struct epoll_event) * kPinkMaxClients);

  firedevent_ = reinterpret_cast<PinkFiredEvent*>(malloc(
      sizeof(PinkFiredEvent) * kPinkMaxClients));
}

PinkEpoll::~PinkEpoll() {
  free(firedevent_);
  free(events_);
  close(epfd_);
}

int PinkEpoll::PinkAddEvent(const int fd, const int mask) {
  struct epoll_event ee;
  ee.data.fd = fd;
  ee.events = mask;
  return epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ee);
}

int PinkEpoll::PinkModEvent(const int fd, const int old_mask, const int mask) {
  struct epoll_event ee;
  ee.data.fd = fd;
  ee.events = (old_mask | mask);
  return epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ee);
}

int PinkEpoll::PinkDelEvent(const int fd) {
  /*
   * Kernel < 2.6.9 need a non null event point to EPOLL_CTL_DEL
   */
  struct epoll_event ee;
  ee.data.fd = fd;
  return epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &ee);
}

int PinkEpoll::PinkPoll(const int timeout) {
  int retval, numevents = 0;
  retval = epoll_wait(epfd_, events_, PINK_MAX_CLIENTS, timeout);
  if (retval > 0) {
    numevents = retval;
    for (int i = 0; i < numevents; i++) {
      int mask = 0;
      firedevent_[i].fd = (events_ + i)->data.fd;

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
