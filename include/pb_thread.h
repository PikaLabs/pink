#ifndef PINK_WORKER_THREAD_H_
#define PINK_WORKER_THREAD_H_

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
#include <google/protobuf/message.h>


template <typename PinkConn>
class WorkerThread : public Thread
{
public:
  WorkerThread() {
    /*
     * install the protobuf handler here
     */
    log_info("WorkerThread construct");
    pink_epoll_ = new PinkEpoll();
    int fds[2];
    if (pipe(fds)) {
      // LOG(FATAL) << "Can't create notify pipe";
      log_err("Can't create notify pipe");
    }
    notify_receive_fd_ = fds[0];
    notify_send_fd_ = fds[1];
    pink_epoll_->PinkAddEvent(notify_receive_fd_, EPOLLIN | EPOLLERR | EPOLLHUP);

  }

  virtual ~WorkerThread() {
  }

  /*
   * The PbItem queue is the fd queue, receive from dispatch thread
   */
  std::queue<PinkItem> conn_queue_;

  int notify_receive_fd() { 
    return notify_receive_fd_; 
  }
  int notify_send_fd() { 
    return notify_send_fd_; 
  }
  Mutex mutex_;

private:
  /*
   * These two fd receive the notify from dispatch thread
   */
  int notify_receive_fd_;
  int notify_send_fd_;

  std::map<int, void *> conns_;

  /*
   * The epoll handler
   */
  PinkEpoll *pink_epoll_;


  virtual void *ThreadMain()
  {
    int nfds;
    PinkFiredEvent *tfe = NULL;
    char bb[1];
    PinkItem ti;
    PinkConn *inConn;
    for (;;) {
      nfds = pink_epoll_->PinkPoll();
      /*
       * log_info("nfds %d", nfds);
       */
      for (int i = 0; i < nfds; i++) {
        tfe = (pink_epoll_->firedevent()) + i;
        log_info("tfe->fd_ %d tfe->mask_ %d", tfe->fd_, tfe->mask_);
        if (tfe->fd_ == notify_receive_fd_ && (tfe->mask_ & EPOLLIN)) {
          read(notify_receive_fd_, bb, 1);
          {
            MutexLock l(&mutex_);
            ti = conn_queue_.front();
            conn_queue_.pop();
          }
          PinkConn *tc = new PinkConn(ti.fd());
          tc->SetNonblock();
          conns_[ti.fd()] = tc;

          pink_epoll_->PinkAddEvent(ti.fd(), EPOLLIN);
          log_info("receive one fd %d", ti.fd());
          /*
           * tc->set_thread(this);
           */
        }
        int shouldClose = 0;
        if (tfe->mask_ & EPOLLIN) {
          inConn = static_cast<PinkConn *>(conns_[tfe->fd_]);
          if (inConn == NULL) {
            continue;
          }
          ReadStatus getRes = inConn->GetRequest();
          if (getRes == kReadAll) {
            pink_epoll_->PinkModEvent(tfe->fd_, 0, EPOLLOUT);
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
            delete(inConn);
            shouldClose = 1;
          }
        }
        if (tfe->mask_ & EPOLLOUT) {
          std::map<int, void *>::iterator iter = conns_.begin();

          if (tfe == NULL) {
            continue;
          }

          iter = conns_.find(tfe->fd_);
          if (iter == conns_.end()) {
            continue;
          }

          inConn = static_cast<PinkConn *>(iter->second);
          WriteStatus write_status = inConn->SendReply();
          if (write_status == kWriteAll) {
            pink_epoll_->PinkModEvent(tfe->fd_, 0, EPOLLIN);
          } else if (write_status == kWriteHalf) {
            continue;
          } else if (write_status == kWriteError) {
            delete(inConn);
            shouldClose = 1;
          }
        }
        if ((tfe->mask_  & EPOLLERR) || (tfe->mask_ & EPOLLHUP)) {
          log_info("close tfe fd here");
          close(tfe->fd_);
        }
        if (shouldClose) {
          log_info("close tfe fd here");
          close(tfe->fd_);
        }
      }
    }
    return NULL;
  }

};

#endif
