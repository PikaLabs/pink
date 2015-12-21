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
#include "pink_socket.h"

class PinkEpoll;
class ServerSocket;

template <typename T>
class DispatchThread : public Thread
{
public:
  // This type Dispatch thread just get Connection and then Dispatch the fd to
  // worker thead
  DispatchThread(int port, int work_num, WorkerThread<T> **worker_thread) :
    work_num_(work_num) 
  {

    worker_thread_ = worker_thread;
    server_socket_ = new ServerSocket(port);

    server_socket_->Listen();
    // init epoll
    pink_epoll_ = new PinkEpoll();
    pink_epoll_->PinkAddEvent(server_socket_->sockfd(), EPOLLIN | EPOLLERR | EPOLLHUP);


    last_thread_ = 0;
    for (int i = 0; i < work_num_; i++) {
      worker_thread_[i]->StartThread();
    }
  }
  // This type dispahch thread will work and Deal the conn
  DispatchThread(int port);
  ~DispatchThread()
  {
  }

  virtual void *ThreadMain()
  {
    int nfds;
    PinkFiredEvent *pfe;
    Status s;
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    int fd, connfd;
    for (;;) {
      nfds = pink_epoll_->PinkPoll();
      pfe = pink_epoll_->firedevent();
      for (int i = 0; i < nfds; i++) {
        fd = (pfe + i)->fd_;
        if (fd == server_socket_->sockfd() && ((pfe + i)->mask_ & EPOLLIN)) {
          connfd = accept(server_socket_->sockfd(), (struct sockaddr *) &cliaddr, &clilen);
          log_info("Accept new fd %d", connfd);
          std::queue<PinkItem> *q = &(worker_thread_[last_thread_]->conn_queue_);
          log_info("pfe must happen");
          PinkItem ti(connfd);
          {
            MutexLock l(&worker_thread_[last_thread_]->mutex_);
            q->push(ti);
          }
          write(worker_thread_[last_thread_]->notify_send_fd(), "", 1);
          last_thread_++;
          last_thread_ %= work_num_;
        } else if ((pfe + i)->mask_ & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
          log_info("Epoll timeout event");
        }
      }
    }
    return NULL;
  }

private:

  /*
   * The tcp server port and address
   */

  ServerSocket *server_socket_;
  
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
  WorkerThread<T> **worker_thread_;

  // No copying allowed
  DispatchThread(const DispatchThread&);
  void operator=(const DispatchThread&);
};

#endif
