#ifndef HOLY_THREAD_H_
#define HOLY_THREAD_H_

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
#include <map>

#include "csapp.h"
#include "xdebug.h"
#include "pink_thread.h"
#include "pink_util.h"
#include "pink_socket.h"
#include "pink_epoll.h"
#include "pink_item.h"
#include "pink_mutex.h"

namespace pink {

template <typename Conn>
class HolyThread: public Thread
{
public:
  // This type thread thread will listen and work self list redis thread
  explicit HolyThread(int port, int cron_interval = 0) :
    Thread::Thread(cron_interval)
  {
    server_socket_ = new ServerSocket(port);

    server_socket_->Listen();
    pthread_rwlock_init(&rwlock_, NULL);
    // init epoll
    pink_epoll_ = new PinkEpoll();
    pink_epoll_->PinkAddEvent(server_socket_->sockfd(), EPOLLIN | EPOLLERR | EPOLLHUP);

  }

  virtual ~HolyThread() {
    server_socket_->Close();
  }

  virtual void CronHandle() {
  }

  virtual bool AccessHandle(const std::string& ip) {
    return true;
  }

  pthread_rwlock_t* rwlock() {
    return &rwlock_;
  }
  std::map<int, void*>* conns() {
    return &conns_;
  }

private:


  /*
   * The tcp server port and address
   */

  ServerSocket *server_socket_;
  
  pthread_rwlock_t rwlock_;
  std::map<int, void *> conns_;

  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;

public:
  virtual void *ThreadMain()
  {
    int nfds;
    PinkFiredEvent *pfe;
    Status s;
    struct sockaddr_in cliaddr;
    socklen_t clilen=sizeof(struct sockaddr);
    int fd, connfd;
    Conn *in_conn;

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

    for (;;) {

      if(cron_interval_ > 0) {
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
        log_info("tfe->fd_ %d tfe->mask_ %d", pfe->fd_, pfe->mask_);
        fd = pfe->fd_;
        if (fd == server_socket_->sockfd() && (pfe->mask_ & EPOLLIN)) {
          if (pfe->mask_ & EPOLLIN) {
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
            Conn *tc = new Conn(connfd, ip_port, this);
            tc->SetNonblock();
            {
            RWLock l(&rwlock_, true);
            conns_[connfd] = tc;
            }

            pink_epoll_->PinkAddEvent(connfd, EPOLLIN);
          } else {
            continue;
          }
        } else {
          int should_close = 0;
          std::map<int, void *>::iterator iter = conns_.begin();
          if (pfe == NULL) {
            continue;
          }
          iter = conns_.find(pfe->fd_);
          if (iter == conns_.end()) {
            continue;
          }

          if (pfe->mask_ & EPOLLIN) {

            in_conn = static_cast<Conn *>(iter->second);
            ReadStatus getRes = in_conn->GetRequest();
            in_conn->set_last_interaction(&now);
            if (getRes != kReadAll && getRes != kReadHalf) {
              // kReadError kReadClose kFullError kParseError
              should_close = 1;
            } else if (in_conn->is_reply()) {
              pink_epoll_->PinkModEvent(pfe->fd_, 0, EPOLLOUT);
            } else {
              continue;
            }
          }

          if (pfe->mask_ & EPOLLOUT) {

            in_conn = static_cast<Conn *>(iter->second);
            WriteStatus write_status = in_conn->SendReply();
            if (write_status == kWriteAll) {
              in_conn->set_is_reply(false);
              pink_epoll_->PinkModEvent(pfe->fd_, 0, EPOLLIN);
            } else if (write_status == kWriteHalf) {
              continue;
            } else if (write_status == kWriteError) {
              delete(in_conn);
              should_close = 1;
            }
          }
          if ((pfe->mask_  & EPOLLERR) || (pfe->mask_ & EPOLLHUP) || should_close) {
            log_info("close pfe fd here");
            {
            RWLock l(&rwlock_, true);
            close(pfe->fd_);
            delete(in_conn);
            conns_.erase(pfe->fd_);
            }
          }
        }
      }
    }
    return NULL;
  }

};
}

#endif
