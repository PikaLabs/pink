#include "src/worker_thread.h"

#include "include/pink_conn.h"
#include "src/pink_item.h"
#include "src/pink_epoll.h"
#include "src/csapp.h"

namespace pink {

WorkerThread::WorkerThread(ConnFactory *conn_factory, int cron_interval) :
  conn_factory_(conn_factory),
  cron_interval_(cron_interval) {
  /*
   * install the protobuf handler here
   */
  pthread_rwlock_init(&rwlock_, NULL);
  pink_epoll_ = new PinkEpoll();
  int fds[2];
  if (pipe(fds)) {
    exit(-1);
  }
  notify_receive_fd_ = fds[0];
  notify_send_fd_ = fds[1];
  pink_epoll_->PinkAddEvent(notify_receive_fd_, EPOLLIN | EPOLLERR | EPOLLHUP);
}

WorkerThread::~WorkerThread() {
  delete(pink_epoll_);
}

void *WorkerThread::ThreadMain() {
  int nfds;
  PinkFiredEvent *pfe = NULL;
  char bb[1];
  PinkItem ti;
  PinkConn *in_conn = NULL;

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
        timeout = cron_interval_;
      }
    }

    nfds = pink_epoll_->PinkPoll(timeout);

    for (int i = 0; i < nfds; i++) {
      pfe = (pink_epoll_->firedevent()) + i;
      if (pfe->fd == notify_receive_fd_) {
        if (pfe->mask & EPOLLIN) {
          read(notify_receive_fd_, bb, 1);
          {
            MutexLock l(&mutex_);
            ti = conn_queue_.front();
            conn_queue_.pop();
          }
          PinkConn *tc = conn_factory_->NewPinkConn(ti.fd(), ti.ip_port(), this);
          tc->SetNonblock();
          {
            RWLock l(&rwlock_, true);
            conns_[ti.fd()] = tc;
          }
          pink_epoll_->PinkAddEvent(ti.fd(), EPOLLIN);
        } else {
          continue;
        }
      } else {
        in_conn = NULL;
        int should_close = 0;
        std::map<int, PinkConn *>::iterator iter = conns_.begin();
        if (pfe == NULL) {
          continue;
        }
        iter = conns_.find(pfe->fd);
        if (iter == conns_.end()) {
          pink_epoll_->PinkDelEvent(pfe->fd);
          continue;
        }

        if (pfe->mask & EPOLLIN) {
          in_conn = iter->second;
          ReadStatus getRes = in_conn->GetRequest();
          in_conn->set_last_interaction(now);
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
          in_conn = iter->second;
          WriteStatus write_status = in_conn->SendReply();
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

void WorkerThread::Cleanup() {
  RWLock l(&rwlock_, true);
  PinkConn *in_conn;
  std::map<int, PinkConn *>::iterator iter = conns_.begin();
  for (; iter != conns_.end(); iter++) {
    close(iter->first);
    in_conn = iter->second;
    delete in_conn;
  }
}

extern Thread *NewWorkerThread(ConnFactory *conn_factory, int cron_interval) {
  return new WorkerThread(conn_factory, cron_interval);
}

};
