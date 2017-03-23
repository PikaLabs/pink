#include "holy_thread.h"

#include "pink_epoll.h"
#include "pink_item.h"
#include "pink/include/pink_conn.h"

namespace pink {

HolyThread::HolyThread(int port, ConnFactory *conn_factory, int cron_interval) :
  ServerThread::ServerThread(port, cron_interval),
  conn_factory_(conn_factory) {
  pthread_rwlock_init(&rwlock_, NULL);
}

HolyThread::HolyThread(const std::string& bind_ip, int port, ConnFactory *conn_factory, int cron_interval) :
  ServerThread::ServerThread(bind_ip, port, cron_interval),
  conn_factory_(conn_factory) {
  pthread_rwlock_init(&rwlock_, NULL);
}

HolyThread::HolyThread(const std::set<std::string>& bind_ips, int port, ConnFactory *conn_factory, int cron_interval) :
  ServerThread::ServerThread(bind_ips, port, cron_interval),
  conn_factory_(conn_factory) {
  pthread_rwlock_init(&rwlock_, NULL);
}

HolyThread::~HolyThread() {
  Cleanup();
}

void HolyThread::HandleNewConn(const int connfd, const std::string &ip_port) {
  PinkConn *tc = conn_factory_->NewPinkConn(connfd, ip_port, this);
  tc->SetNonblock();
  {
    RWLock l(&rwlock_, true);
    conns_[connfd] = tc;
  }

  pink_epoll_->PinkAddEvent(connfd, EPOLLIN);
}

void HolyThread::HandleConnEvent(PinkFiredEvent *pfe) {
  if (pfe == NULL) {
    return;
  }
  PinkConn *in_conn = NULL;
  int should_close = 0;
  std::map<int, PinkConn *>::iterator iter;
  {
    RWLock l(&rwlock_, false);
    if ((iter = conns_.find(pfe->fd)) == conns_.end()) {
      pink_epoll_->PinkDelEvent(pfe->fd);
      return;
    }
  }
  in_conn = iter->second;
  if (pfe->mask & EPOLLIN) {
    ReadStatus getRes = in_conn->GetRequest();
    struct timeval now;
    gettimeofday(&now, NULL);
    in_conn->set_last_interaction(now);
    if (getRes != kReadAll && getRes != kReadHalf) {
      // kReadError kReadClose kFullError kParseError
      should_close = 1;
    } else if (in_conn->is_reply()) {
      pink_epoll_->PinkModEvent(pfe->fd, 0, EPOLLOUT);
    } else {
      return;
    }
  }
  if (pfe->mask & EPOLLOUT) {
    WriteStatus write_status = in_conn->SendReply();
    if (write_status == kWriteAll) {
      in_conn->set_is_reply(false);
      pink_epoll_->PinkModEvent(pfe->fd, 0, EPOLLIN);
    } else if (write_status == kWriteHalf) {
      return;
    } else if (write_status == kWriteError) {
      should_close = 1;
    }
  }
  if ((pfe->mask & EPOLLERR) || (pfe->mask & EPOLLHUP) || should_close) {
    pink_epoll_->PinkDelEvent(pfe->fd);
    close(pfe->fd);
    delete(in_conn);
    in_conn = NULL;

    RWLock l(&rwlock_, true);
    conns_.erase(pfe->fd);
  }
}

// clean conns
void HolyThread::Cleanup() {
  RWLock l(&rwlock_, true);
  PinkConn *in_conn;
  std::map<int, PinkConn *>::iterator iter = conns_.begin();
  for (; iter != conns_.end(); iter++) {
    close(iter->first);
    in_conn = iter->second;
    delete in_conn;
  }
}

extern ServerThread *NewHolyThread(int port, ConnFactory *conn_factory, int cron_interval) {
  return new HolyThread(port, conn_factory, cron_interval);
}

extern ServerThread *NewHolyThread(const std::string &bind_ip, int port,
    ConnFactory *conn_factory, int cron_interval) {
  return new HolyThread(bind_ip, port, conn_factory, cron_interval);
}

extern ServerThread *NewHolyThread(const std::set<std::string>& bind_ips, 
    int port, ConnFactory *conn_factory, int cron_interval) {
  return new HolyThread(bind_ips, port, conn_factory, cron_interval);
}
};  // namespace pink
