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

namespace pink {

template <typename Conn>
class WorkerThread : public Thread
{
public:
  WorkerThread(int cron_interval = 0):
    Thread::Thread(cron_interval)
  {
    /*
     * install the protobuf handler here
     */
    log_info("WorkerThread construct");
    pthread_rwlock_init(&rwlock_, NULL);
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
    delete(pink_epoll_);
  }

  virtual void CronHandle() {
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

  pthread_rwlock_t* rwlock() {
    return &rwlock_;
  }
  std::map<int, void *> conns() {
    return conns_;
  }

private:
  /*
   * These two fd receive the notify from dispatch thread
   */
  int notify_receive_fd_;
  int notify_send_fd_;
 

  pthread_rwlock_t rwlock_;
  std::map<int, void *> conns_;

  /*
   * The epoll handler
   */
  PinkEpoll *pink_epoll_;


  virtual void *ThreadMain()
  {
    int nfds;
    PinkFiredEvent *pfe = NULL;
    char bb[1];
    PinkItem ti;
    Conn *in_conn;

    struct timeval when;
    gettimeofday(&when, NULL);
    struct timeval now = when;

    when.tv_sec += (cron_interval_ / 1000);
    when.tv_usec += ((cron_interval_ % 1000 ) * 1000);
    int timeout = cron_interval_;
    if (timeout <= 0 ) {
      timeout = PINK_CRON_INTERVAL;
    }

    for (;;) {

      if (cron_interval_ > 0 ) {
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
       * log_info("nfds %d", nfds);
       */
      for (int i = 0; i < nfds; i++) {
        pfe = (pink_epoll_->firedevent()) + i;
        log_info("pfe->fd_ %d pfe->mask_ %d", pfe->fd_, pfe->mask_);
        if (pfe->fd_ == notify_receive_fd_) {
          if (pfe->mask_ & EPOLLIN) {
            read(notify_receive_fd_, bb, 1);
            {
              MutexLock l(&mutex_);
              ti = conn_queue_.front();
              conn_queue_.pop();
            }
            Conn *tc = new Conn(ti.fd(), ti.ip_port(), this);
            tc->SetNonblock();
            {
            RWLock l(&rwlock_, true);
            conns_[ti.fd()] = tc;
            }
            pink_epoll_->PinkAddEvent(ti.fd(), EPOLLIN);
            log_info("receive one fd %d", ti.fd());
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
            log_info("now: %d, %d", now.tv_sec, now.tv_usec);
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
