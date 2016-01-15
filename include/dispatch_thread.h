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
#include <sys/time.h>

#include "csapp.h"
#include "xdebug.h"
#include "pink_thread.h"
#include "worker_thread.h"
#include "pink_util.h"
#include "pink_socket.h"
#include "pink_epoll.h"

template <typename T>
class DispatchThread : public Thread
{
public:
  // This type Dispatch thread just get Connection and then Dispatch the fd to
  // worker thead
  DispatchThread(int port, int work_num, WorkerThread<T> **worker_thread, int cron_interval = 0) :
    Thread::Thread(cron_interval),
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

  int work_num() {
    return work_num_;
  }

  WorkerThread<T>** worker_thread() {
    return worker_thread_;
  }

  virtual ~DispatchThread()
  {
    delete(pink_epoll_);
    server_socket_->Close();
  }

  virtual void CronHandle() {
  }

  virtual bool AccessHandle(std::string& ip) {
    return true;
  }

  virtual void *ThreadMain()
  {
    int nfds;
    PinkFiredEvent *pfe;
    Status s;
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(struct sockaddr);
    int fd, connfd;

    struct timeval when;
    struct timeval now;
    gettimeofday(&when, NULL);

    when.tv_sec += (cron_interval_ / 1000);
    when.tv_usec += ((cron_interval_ % 1000 ) * 1000);
    int timeout = cron_interval_;
    if (timeout <= 0) {
      timeout = PINK_CRON_INTERVAL;
    }

    std::string ip_port;
    char port_buf[32];
    char ip_addr[INET_ADDRSTRLEN] = "";

    for (;;) {

      if (cron_interval_ > 0 ) {
        gettimeofday(&now, NULL);
        if (when.tv_sec > now.tv_sec || (when.tv_sec == now.tv_sec && when.tv_usec > now.tv_usec)) {
          timeout = (when.tv_sec - now.tv_sec) * 1000 + (when.tv_usec - now.tv_usec) / 1000;
        } else {
          CronHandle();
          timeout = cron_interval_;
        }
      }

      nfds = pink_epoll_->PinkPoll(timeout);
      for (int i = 0; i < nfds; i++) {
        pfe = (pink_epoll_->firedevent()) + i;
        fd = pfe->fd_;
        if (fd == server_socket_->sockfd() && (pfe->mask_ & EPOLLIN)) {
          connfd = accept(server_socket_->sockfd(), (struct sockaddr *) &cliaddr, &clilen);
          if (connfd == -1) {
            if (errno != EWOULDBLOCK) {
                continue;
            }
          }

          ip_port = inet_ntop(AF_INET, &cliaddr.sin_addr, ip_addr, sizeof(ip_addr));
          if (!AccessHandle(ip_port)) {
            close(connfd);
            continue;
          }

          ip_port.append(":");
          sprintf(port_buf, "%d", ntohs(cliaddr.sin_port));
          ip_port.append(port_buf);

          log_info("Accept new fd %d", connfd);
          std::queue<PinkItem> *q = &(worker_thread_[last_thread_]->conn_queue_);
          PinkItem ti(connfd, ip_port);
          {
            MutexLock l(&worker_thread_[last_thread_]->mutex_);
            q->push(ti);
          }
          write(worker_thread_[last_thread_]->notify_send_fd(), "", 1);
          last_thread_++;
          last_thread_ %= work_num_;
        } else if (pfe->mask_ & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
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
//  int cron_interval_;
  /*
   * This is the work threads
   */
  WorkerThread<T> **worker_thread_;

  // No copying allowed
  DispatchThread(const DispatchThread&);
  void operator=(const DispatchThread&);
};

#endif
