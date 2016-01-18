#ifndef PINK_EPOLL_H__
#define PINK_EPOLL_H__
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
