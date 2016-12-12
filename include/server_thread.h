#ifndef SERVER_THREAD_H_
#define SERVER_THREAD_H_

#include <sys/epoll.h>

#include <set>
#include <vector>

#include "include/csapp.h"
#include "include/status.h"
#include "include/pink_define.h"
#include "include/pink_thread.h"
#include "include/pink_socket.h"
#include "include/pink_epoll.h"
#include "include/pink_mutex.h"

namespace pink {

class ServerThread : public Thread {
 public:
  explicit ServerThread(int port, int cron_interval = 0) :
    cron_interval_(cron_interval),
    port_(port) {
    ips_.insert("0.0.0.0");
  }

  ServerThread(const std::string& bind_ip, int port, int cron_interval = 0) :
    cron_interval_(cron_interval),
    port_(port) {
    ips_.insert(bind_ip);
  }

  ServerThread(const std::set<std::string>& bind_ips, int port, int cron_interval = 0) :
    cron_interval_(cron_interval),
    port_(port) {
    ips_ = bind_ips;
  }

  virtual ~ServerThread() {
    should_exit_ = true;
    JoinThread();

    delete(pink_epoll_);
    for (std::vector<ServerSocket*>::iterator iter = server_sockets_.begin();
        iter != server_sockets_.end();
        ++iter) {
      delete *iter;
    }
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
      pink_epoll_->PinkAddEvent(socket_p->sockfd(), EPOLLIN | EPOLLERR | EPOLLHUP);
      server_sockets_.push_back(socket_p);
      server_fds_.insert(socket_p->sockfd());
    }
    return kSuccess;
  
  }

  virtual void CronHandle() {
  }

  virtual bool AccessHandle(std::string& ip) {
    return true;
  }

  virtual void *ThreadMain() {
    int nfds;
    PinkFiredEvent *pfe;
    Status s;
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(struct sockaddr);
    int fd, connfd;

    struct timeval when;
    gettimeofday(&when, NULL);
    struct timeval now = when;

    when.tv_sec += (cron_interval_ / 1000);
    when.tv_usec += ((cron_interval_ % 1000 ) * 1000);
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
      for (int i = 0; i < nfds; i++) {
        pfe = (pink_epoll_->firedevent()) + i;
        log_info("tfe->fd %d tfe->mask %d", pfe->fd, pfe->mask);
        fd = pfe->fd;
        /*
         * Handle server event
         */
        if (server_fds_.find(fd) != server_fds_.end()) {
          if (pfe->mask & EPOLLIN) {
            connfd = accept(fd, (struct sockaddr *) &cliaddr, &clilen);
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

            /*
             * Handle new connection,
             * implemented in derived class
             */
            HandleNewConn(connfd, ip_port);

          } else if (pfe->mask & (EPOLLHUP | EPOLLERR)) {
            /*
             * this branch means there is error on the listen fd
             */
            log_info("close the fd here");
            close(pfe->fd);
            continue;
          }
        } else {
          /*
           * Handle connection's event
           * implemented in derived class
           */
          HandleConnEvent(pfe);
        }
      }
    }

    return NULL;
  }

 protected:
  int cron_interval_;
  /*
   * The tcp server port and address
   */

  std::vector<ServerSocket*> server_sockets_;
  std::set<int32_t> server_fds_;

  int port_;
  std::set<std::string> ips_;

  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;

  /*
   * The server connection and event handle
   */
  virtual void HandleNewConn(const int& connfd, const std::string& ip_port) {
  }
  virtual void HandleConnEvent(PinkFiredEvent *pfe) {
  }
};
} // namespace pink

#endif
