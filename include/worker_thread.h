// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_WORKER_THREAD_H_
#define PINK_INCLUDE_WORKER_THREAD_H_

#include <sys/epoll.h>

#include <string>
#include <functional>
#include <queue>
#include <map>

#include <google/protobuf/message.h>

#include "include/pink_thread.h"
#include "src/pink_item.h"
#include "src/pink_epoll.h"
#include "include/pink_mutex.h"
#include "include/pink_define.h"
#include "src/csapp.h"
#include "include/xdebug.h"

namespace pink {

template <typename Conn>
class WorkerThread : public Thread {
 public:
  explicit WorkerThread(int cron_interval);
  virtual ~WorkerThread();

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
  PinkEpoll* pink_epoll() {
    return pink_epoll_;
  }
  Mutex mutex_;

  /*
   *  public for external statistics
   */
  pthread_rwlock_t rwlock_;
  std::map<int, void *> conns_;


 private:
  int cron_interval_;
  /*
   * These two fd receive the notify from dispatch thread
   */
  int notify_receive_fd_;
  int notify_send_fd_;

  /*
   * The epoll handler
   */
  PinkEpoll *pink_epoll_;

  virtual void *ThreadMain() override;

  void CronHandle() {}

  // clean conns
  void Cleanup();

};  // class WorkerThread

template <typename Conn>
WorkerThread<Conn>::WorkerThread(int cron_interval = 0) :
  cron_interval_(cron_interval) {
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

template <typename Conn>
WorkerThread<Conn>::~WorkerThread() {
  delete(pink_epoll_);
}

template <typename Conn>
void *WorkerThread<Conn>::ThreadMain() {
  int nfds;
  PinkFiredEvent *pfe = NULL;
  char bb[1];
  PinkItem ti;
  Conn *in_conn = NULL;

  struct timeval when;
  gettimeofday(&when, NULL);
  struct timeval now = when;

  when.tv_sec += (cron_interval_ / 1000);
  when.tv_usec += ((cron_interval_ % 1000 ) * 1000);
  int timeout = cron_interval_;
  if (timeout <= 0) {
    timeout = PINK_CRON_INTERVAL;
  }

  while (running()) {

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
      log_info("pfe->fd_ %d pfe->mask_ %d", pfe->fd, pfe->mask);
      if (pfe->fd == notify_receive_fd_) {
        if (pfe->mask & EPOLLIN) {
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
        in_conn = NULL;
        int should_close = 0;
        std::map<int, void *>::iterator iter = conns_.begin();
        if (pfe == NULL) {
          continue;
        }
        iter = conns_.find(pfe->fd);
        if (iter == conns_.end()) {
          pink_epoll_->PinkDelEvent(pfe->fd);
          continue;
        }

        if (pfe->mask & EPOLLIN) {
          in_conn = static_cast<Conn *>(iter->second);
          ReadStatus getRes = in_conn->GetRequest();
          in_conn->set_last_interaction(now);
          log_info("now: %d, %d", now.tv_sec, now.tv_usec);
          log_info("in_conn->is_reply() %d", in_conn->is_reply());
          if (getRes != kReadAll && getRes != kReadHalf) {
            // kReadError kReadClose kFullError kParseError
            should_close = 1;
          } else if (in_conn->is_reply()) {
            pink_epoll_->PinkModEvent(pfe->fd, EPOLLIN, EPOLLOUT);
          } else {
            continue;
          }
        }
        if (pfe->mask & EPOLLOUT) {
          in_conn = static_cast<Conn *>(iter->second);
          log_info("in work thead SendReply before");
          WriteStatus write_status = in_conn->SendReply();
          log_info("in work thead SendReply after");
          if (write_status == kWriteAll) {
            in_conn->set_is_reply(false);
            pink_epoll_->PinkModEvent(pfe->fd, 0, EPOLLIN);
          } else if (write_status == kWriteHalf) {
            continue;
          } else if (write_status == kWriteError) {
            should_close = 1;
          }
        }
        if ((pfe->mask & EPOLLERR) || (pfe->mask & EPOLLHUP) || should_close) {
          log_info("close pfe fd here");
          {
            RWLock l(&rwlock_, true);
            pink_epoll_->PinkDelEvent(pfe->fd);
            close(pfe->fd);
            delete(in_conn);
            in_conn = NULL;

            conns_.erase(pfe->fd);
          }
        }
      } // connection event
    } // for (int i = 0; i < nfds; i++)
  } // while (running())

  Cleanup();
  return NULL;
}

template <typename Conn>
void WorkerThread<Conn>::Cleanup() {
  RWLock l(&rwlock_, true);
  Conn *in_conn;
  std::map<int, void *>::iterator iter = conns_.begin();
  for (; iter != conns_.end(); iter++) {
    close(iter->first);
    in_conn = static_cast<Conn *>(iter->second);
    delete in_conn;
  }
}

}  // namespace pink

#endif  // PINK_INCLUDE_WORKER_THREAD_H_
