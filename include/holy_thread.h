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

template <typename PinkConn>
class HolyThread: public Thread
{
public:
  // This type thread thread will listen and work self list redis thread
  explicit HolyThread(int port)
  {
    server_socket_ = new ServerSocket(port);

    server_socket_->Listen();
    // init epoll
    pink_epoll_ = new PinkEpoll();
    pink_epoll_->PinkAddEvent(server_socket_->sockfd(), EPOLLIN | EPOLLERR | EPOLLHUP);

  }

  ~HolyThread()
  {
    server_socket_->Close();
  }
private:

  std::map<int, void *> conns_;

  /*
   * The tcp server port and address
   */

  ServerSocket *server_socket_;
  
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
    socklen_t clilen;
    int fd, connfd;
    PinkConn *in_conn;
    for (;;) {
      nfds = pink_epoll_->PinkPoll();
      for (int i = 0; i < nfds; i++) {
        pfe = (pink_epoll_->firedevent()) + i;
        log_info("tfe->fd_ %d tfe->mask_ %d", pfe->fd_, pfe->mask_);
        fd = pfe->fd_;
        if (fd == server_socket_->sockfd() && (pfe->mask_ & EPOLLIN)) {
          connfd = accept(server_socket_->sockfd(), (struct sockaddr *) &cliaddr, &clilen);
          log_info("Accept new fd %d", connfd);
          
          PinkConn *tc = new PinkConn(connfd);
          tc->SetNonblock();
          conns_[connfd] = tc;

          pink_epoll_->PinkAddEvent(connfd, EPOLLIN);
        } else {
          int should_close = 0;
          if (pfe->mask_ & EPOLLIN) {
            in_conn = static_cast<PinkConn *>(conns_[pfe->fd_]);
            if (in_conn == NULL) {
              continue;
            }
            ReadStatus getRes = in_conn->GetRequest();
            if (getRes == kReadAll) {
              pink_epoll_->PinkModEvent(pfe->fd_, 0, EPOLLOUT);
            } else if (getRes == kReadHalf) {
              /*
               * This branch is that wo don't read all the data or we come into the EAGAIN branch
               */
              continue;
            } else if (getRes == kReadError || getRes == kReadClose) {
              /*
               * In this branch we need close the fd, there is two situation
               * 1. there is some error on this fd
               * 2. the client has send the close operator
               *
               */
              delete(in_conn);
              should_close = 1;
            }
          }
          if (pfe->mask_ & EPOLLOUT) {
            std::map<int, void *>::iterator iter = conns_.begin();

            if (pfe == NULL) {
              continue;
            }

            iter = conns_.find(pfe->fd_);
            if (iter == conns_.end()) {
              continue;
            }

            in_conn = static_cast<PinkConn *>(iter->second);
            WriteStatus write_status = in_conn->SendReply();
            if (write_status == kWriteAll) {
              pink_epoll_->PinkModEvent(pfe->fd_, 0, EPOLLIN);
            } else if (write_status == kWriteHalf) {
              continue;
            } else if (write_status == kWriteError) {
              delete(in_conn);
              should_close = 1;
            }
          }
          if ((pfe->mask_  & EPOLLERR) || (pfe->mask_ & EPOLLHUP)) {
            log_info("close pfe fd here");
            close(pfe->fd_);
          }
          if (should_close) {
            log_info("close pfe fd here");
            close(pfe->fd_);
          }
        }
      }
    }
    return NULL;
  }

};

#endif
