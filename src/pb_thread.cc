#include "pb_thread.h"
#include "pink_mutex.h"
#include "csapp.h"
#include "xdebug.h"
#include "pink_epoll.h"
#include <sys/epoll.h>
#include "pb_conn.h"

PbThread::PbThread()
{
  /*
   * install the protobuf handler here
   */
  log_info("pbthread construct");
  pinkEpoll_ = new PinkEpoll();
  int fds[2];
  if (pipe(fds)) {
    // LOG(FATAL) << "Can't create notify pipe";
    log_err("Can't create notify pipe");
  }
  notify_receive_fd_ = fds[0];
  notify_send_fd_ = fds[1];
  pinkEpoll_->PinkAddEvent(notify_receive_fd_, EPOLLIN | EPOLLERR | EPOLLHUP);

}

PbThread::~PbThread()
{

}


void *PbThread::ThreadMain()
{
  int nfds;
  PinkFiredEvent *tfe = NULL;
  char bb[1];
  PinkItem ti;
  PbConn *inConn;
  for (;;) {
    nfds = pinkEpoll_->PinkPoll();
    /*
     * log_info("nfds %d", nfds);
     */
    for (int i = 0; i < nfds; i++) {
      tfe = (pinkEpoll_->firedevent()) + i;
      log_info("tfe->fd_ %d tfe->mask_ %d", tfe->fd_, tfe->mask_);
      if (tfe->fd_ == notify_receive_fd_ && (tfe->mask_ & EPOLLIN)) {
        read(notify_receive_fd_, bb, 1);
        {
        MutexLock l(&mutex_);
        ti = conn_queue_.front();
        conn_queue_.pop();
        }
        PbConn *tc = new PbConn(ti.fd(), this);
        tc->SetNonblock();
        conns_[ti.fd()] = tc;

        pinkEpoll_->PinkAddEvent(ti.fd(), EPOLLIN);
        log_info("receive one fd %d", ti.fd());
        /*
         * tc->set_thread(this);
         */
      }
      int shouldClose = 0;
      if (tfe->mask_ & EPOLLIN) {
        inConn = conns_[tfe->fd_];
        if (inConn == NULL) {
          continue;
        }
        ReadStatus getRes = inConn->PbGetRequest();
        if (getRes == kReadAll) {
          pinkEpoll_->PinkModEvent(tfe->fd_, 0, EPOLLOUT);
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
        std::map<int, PbConn *>::iterator iter = conns_.begin();

        if (tfe == NULL) {
          continue;
        }
        
        iter = conns_.find(tfe->fd_);
        if (iter == conns_.end()) {
          continue;
        }

        inConn = iter->second;
        WriteStatus writeStatus = inConn->PbSendReply();
        if (writeStatus == kWriteAll) {
          pinkEpoll_->PinkModEvent(tfe->fd_, 0, EPOLLIN);
        } else if (writeStatus == kWriteHalf) {
          continue;
        } else if (writeStatus == kWriteError) {
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
