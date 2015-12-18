#ifndef DISPATCH_THREAD_H_
#define DISPATCH_THREAD_H_

#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <event.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "csapp.h"
#include "xdebug.h"
#include "pink_thread.h"
#include "pb_thread.h"
#include "pink_util.h"

class PinkEpoll;

class DispatchThread : public Thread
{
public:
  DispatchThread(int port, int work_num, PbThread **pbThread);
  ~DispatchThread();

  virtual void *ThreadMain();

private:

  /*
   * The tcp server port and address
   */
  int sockfd_;
  int flags_;
  int port_;
  struct sockaddr_in servaddr_;
  
  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;

  /*
   * Here we used auto poll to find the next work thread,
   * last_thread_ is the last work thread
   */
  int last_thread_;

  int work_num_;
  /*
   * This is the work threads
   */
  PbThread **pbThread_;

  // No copying allowed
  DispatchThread(const DispatchThread&);
  void operator=(const DispatchThread&);
};

#endif
