#include "pink_epoll.h"
#include "pink_define.h"
#include "xdebug.h"
#include "status.h"

namespace pink {

PinkEpoll::PinkEpoll()
{
  epfd_ = epoll_create(1024); 
  events_ = (struct epoll_event *)malloc(sizeof(struct epoll_event) * PINK_MAX_CLIENTS);
  if (!events_) {
    log_err("init epoll_event error");
  }
  timeout_ = 1000;

  firedevent_ = (PinkFiredEvent *)malloc(sizeof(PinkFiredEvent) * PINK_MAX_CLIENTS);
}

PinkEpoll::~PinkEpoll()
{
  free(events_);
  close(epfd_);
}

Status PinkEpoll::PinkAddEvent(int fd, int mask)
{
  struct epoll_event ee;
  ee.data.fd = fd;
  ee.events = mask;
  if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ee) == -1) {
    return Status::Corruption("epollAdd error");
  }
  return Status::OK();
}


Status PinkEpoll::PinkModEvent(int fd, int oMask, int mask)
{
  struct epoll_event ee;
  ee.data.fd = fd;
  ee.events = (oMask | mask);
  if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ee) == -1) {
    return Status::Corruption("epollCtl error");
  }
  return Status::OK();
}

void PinkEpoll::PinkDelEvent(int fd)
{
  /*
   * Kernel < 2.6.9 need a non null event point to EPOLL_CTL_DEL
   */
  struct epoll_event ee;
  ee.data.fd = fd;
  epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &ee);
}

int PinkEpoll::PinkPoll(int timeout)
{
  int retval, numevents = 0;
  retval = epoll_wait(epfd_, events_, PINK_MAX_CLIENTS, timeout);
  if (retval > 0) {
    numevents = retval;
    for (int i = 0; i < numevents; i++) {
      int mask = 0;
      firedevent_[i].fd_ = (events_ + i)->data.fd;
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
      firedevent_[i].mask_ = mask;
    }
  }
  return numevents;
}

}
