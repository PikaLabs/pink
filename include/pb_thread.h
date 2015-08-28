#ifndef PINK_PB_THREAD_H_
#define PINK_PB_THREAD_H_

#include <string>
#include <functional>
#include <queue>
#include <map>

#include "pink_thread.h"
#include "pink_item.h"
#include "pink_epoll.h"
#include "pink_mutex.h"
#include "pink_define.h"

#include "csapp.h"
#include "xdebug.h"
#include <sys/epoll.h>


class PbConn;

class PbThread : public Thread
{
public:
  PbThread();

  virtual ~PbThread();

  /*
   * The PbItem queue is the fd queue, receive from dispatch thread
   */
  std::queue<PinkItem> conn_queue_;

  int notify_receive_fd() { return notify_receive_fd_; }
  int notify_send_fd() { return notify_send_fd_; }
  Mutex mutex_;




  virtual int DealMessage(const char* buf, const int len) = 0;

private:
  virtual void *ThreadMain();




  /*
   * These two fd receive the notify from dispatch thread
   */
  int notify_receive_fd_;
  int notify_send_fd_;



  std::map<int, PbConn *> conns_;

  /*
   * The epoll handler
   */
  PinkEpoll *pinkEpoll_;
};

#endif
