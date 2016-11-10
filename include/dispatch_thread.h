#ifndef INCLUDE_DISPATCH_THREAD_H_
#define INCLUDE_DISPATCH_THREAD_H_

#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <set>
#include <vector>
#include <queue>

#include "include/csapp.h"
#include "include/xdebug.h"
#include "include/pink_thread.h"
#include "include/worker_thread.h"
#include "include/pink_util.h"
#include "include/pink_socket.h"
#include "include/pink_epoll.h"

namespace pink {
template <typename T>
class DispatchThread : public Thread
{
public:
  // This type Dispatch thread just get Connection and then Dispatch the fd to
  // worker thead
  /**
   * @brief
   *
   * @param port the port number
   * @param work_num
   * @param worker_thread the worker thred we define
   * @param cron_interval the cron job interval
   */
  DispatchThread(int port, int work_num, WorkerThread<T> **worker_thread, int cron_interval = 0) :
    Thread::Thread(cron_interval),
    port_(port),
    work_num_(work_num),
    worker_thread_(worker_thread)
  {
    ips_.insert("0.0.0.0");
  }

  /**
   * @brief
   *
   * @param ip the ip string
   * @param port the port number
   * @param work_num
   * @param worker_thread the worker thred we define
   * @param cron_interval the cron job interval
   */

  DispatchThread(const std::string &ip, int port, int work_num, WorkerThread<T> **worker_thread, int cron_interval = 0) :
    Thread::Thread(cron_interval),
    port_(port),
    work_num_(work_num),
    worker_thread_(worker_thread)
  {
    ips_.insert(ip);
  }

  DispatchThread(const std::set<std::string>& ips, int port, int work_num, WorkerThread<T> **worker_thread, int cron_interval = 0) :
      Thread::Thread(cron_interval),
      port_(port),
      work_num_(work_num),
      worker_thread_(worker_thread)
  {
    ips_ = ips;
  }

  virtual int InitHandle() {
    int ret = 0;
    ServerSocket* socket_p;
    pink_epoll_ = new PinkEpoll();
    if (ips_.find("0.0.0.0") != ips_.end()) {
      ips_.clear();
      ips_.insert("0.0.0.0");
    }
    for (std::set<std::string>::iterator iter = ips_.begin();
        iter != ips_.end();
        ++iter) {
      socket_p = new ServerSocket(port_);
      ret = socket_p->Listen(*iter);
      if (ret != kSuccess) {
        return ret;
      }
      // init epoll
      pink_epoll_->PinkAddEvent(socket_p->sockfd(), EPOLLIN | EPOLLERR | EPOLLHUP);
      server_sockets_.push_back(socket_p);
      server_fds_.insert(socket_p->sockfd());
    }
    last_thread_ = 0;
    for (int i = 0; i < work_num_; i++) {
      worker_thread_[i]->StartThread();
    }
    return kSuccess;
  
  }

  int work_num() {
    return work_num_;
  }

  WorkerThread<T>** worker_thread() {
    return worker_thread_;
  }

  virtual ~DispatchThread() {
    should_exit_ = true;

    JoinThread();

    delete(pink_epoll_);
    for (std::vector<ServerSocket*>::iterator iter = server_sockets_.begin();
        iter != server_sockets_.end();
        ++iter) {
      delete *iter;
    }
  }

  virtual void CronHandle() {
  }

  virtual bool AccessHandle(const std::string& ip) {
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
    when.tv_usec += ((cron_interval_ % 1000) * 1000);
    int timeout = cron_interval_;
    if (timeout <= 0) {
      timeout = PINK_CRON_INTERVAL;
    }

    std::string ip_port;
    char port_buf[32];
    char ip_addr[INET_ADDRSTRLEN] = "";

    while (!should_exit_) {

      if (cron_interval_ > 0) {
        gettimeofday(&now, NULL);
        if (when.tv_sec > now.tv_sec || (when.tv_sec == now.tv_sec && when.tv_usec > now.tv_usec)) {
          timeout = (when.tv_sec - now.tv_sec) * 1000 + (when.tv_usec - now.tv_usec) / 1000;
        } else {
          when.tv_sec = now.tv_sec + (cron_interval_ / 1000);
          when.tv_usec = now.tv_usec + ((cron_interval_ % 1000 ) * 1000);
          CronHandle();
          timeout = cron_interval_;
        }
      }

      nfds = pink_epoll_->PinkPoll(timeout);
      /*
       * we just have the listen socket fd in the epoll
       */
      for (int i = 0; i < nfds; i++) {
        pfe = (pink_epoll_->firedevent()) + i;
        fd = pfe->fd_;
        log_info("come fd is %d\n", fd);
        if (server_fds_.find(fd) != server_fds_.end()) {
          if (pfe->mask_ & EPOLLIN) {
            connfd = accept(fd, (struct sockaddr *) &cliaddr, &clilen);
            log_info("Connect fd %d", connfd);
            if (connfd == -1) {
              if (errno != EWOULDBLOCK) {
                continue;
              }
            }
            fcntl(connfd, F_SETFD, fcntl(connfd, F_GETFD) | FD_CLOEXEC);

            ip_port = inet_ntop(AF_INET, &cliaddr.sin_addr, ip_addr, sizeof(ip_addr));
            if (!AccessHandle(ip_port)) {
              close(connfd);
              continue;
            }

            ip_port.append(":");
            snprintf(port_buf, sizeof(port_buf), "%d", ntohs(cliaddr.sin_port));
            ip_port.append(port_buf);
            std::queue<PinkItem> *q = &(worker_thread_[last_thread_]->conn_queue_);
            PinkItem ti(connfd, ip_port);
            {
              MutexLock l(&worker_thread_[last_thread_]->mutex_);
              q->push(ti);
            }
            write(worker_thread_[last_thread_]->notify_send_fd(), "", 1);
            last_thread_++;
            last_thread_ %= work_num_;
          } else if (pfe->mask_ & (EPOLLERR | EPOLLHUP)) {
            /*
             * this branch means there is error on the listen fd
             */
            log_info("close the fd here");
            close(fd);
          }
        } else {
          continue;
        }
      }
    }
    return NULL;
  }

 private:
  /*
   * The tcp server port and address
   */

  std::vector<ServerSocket*> server_sockets_;
  std::set<int32_t> server_fds_;
  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;

  /*
   * Here we used auto poll to find the next work thread,
   * last_thread_ is the last work thread
   */
  int last_thread_;

  int port_;
  int work_num_;
//  int cron_interval_;
  /*
   * This is the work threads
   */
  WorkerThread<T> **worker_thread_;
  std::set<std::string> ips_;

  // No copying allowed
  DispatchThread(const DispatchThread&);
  void operator=(const DispatchThread&);
};

}// namespace pink

#endif // INCLUDE_DISPATCH_THREAD_H_
