#include "pink/src/holy_thread.h"

#include "pink/src/pink_epoll.h"
#include "pink/src/pink_item.h"
#include "pink/include/pink_conn.h"

namespace pink {

HolyThread::HolyThread(int port,
                       ConnFactory* conn_factory,
                       int cron_interval, const ServerHandle* handle)
    : ServerThread::ServerThread(port, cron_interval, handle),
      conn_factory_(conn_factory),
      keepalive_timeout_(kDefaultKeepAliveTime) {
}

HolyThread::HolyThread(const std::string& bind_ip, int port,
                       ConnFactory* conn_factory,
                       int cron_interval, const ServerHandle* handle)
    : ServerThread::ServerThread(bind_ip, port, cron_interval, handle),
      conn_factory_(conn_factory) {
}

HolyThread::HolyThread(const std::set<std::string>& bind_ips, int port,
                       ConnFactory* conn_factory,
                       int cron_interval, const ServerHandle* handle)
    : ServerThread::ServerThread(bind_ips, port, cron_interval, handle),
      conn_factory_(conn_factory) {
}

HolyThread::~HolyThread() {
  Cleanup();
}

int HolyThread::StartThread() {
  int ret = handle_->CreateWorkerSpecificData(&private_data_);
  if (ret != 0) {
    return ret;
  }
  return ServerThread::StartThread();
}

int HolyThread::StopThread() {
  int ret = handle_->DeleteWorkerSpecificData(private_data_);
  if (ret != 0) {
    return ret;
  }
  return ServerThread::StopThread();
}

void HolyThread::HandleNewConn(const int connfd, const std::string &ip_port) {
  PinkConn *tc = conn_factory_->NewPinkConn(connfd, ip_port, this, private_data_);
  tc->SetNonblock();
  {
    slash::WriteLock l(&rwlock_);
    conns_[connfd] = tc;
  }

  pink_epoll_->PinkAddEvent(connfd, EPOLLIN);
}

void HolyThread::HandleConnEvent(PinkFiredEvent *pfe) {
  if (pfe == nullptr) {
    return;
  }
  PinkConn *in_conn = nullptr;
  int should_close = 0;
  std::map<int, PinkConn *>::iterator iter;
  {
    slash::ReadLock l(&rwlock_);
    if ((iter = conns_.find(pfe->fd)) == conns_.end()) {
      pink_epoll_->PinkDelEvent(pfe->fd);
      return;
    }
  }
  in_conn = iter->second;
  if (pfe->mask & EPOLLIN) {
    ReadStatus getRes = in_conn->GetRequest();
    struct timeval now;
    gettimeofday(&now, nullptr);
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
    in_conn = nullptr;

    slash::WriteLock l(&rwlock_);
    conns_.erase(pfe->fd);
  }
}

void HolyThread::DoCronTask() {
  struct timeval now;
  gettimeofday(&now, NULL);
  slash::WriteLock l(&rwlock_);
  std::map<int, PinkConn*>::iterator iter = conns_.begin();
  while (iter != conns_.end()) {
    // KillConn
    {
    slash::MutexLock l(&killer_mutex_);
    if (deleting_conn_ipport_.count(iter->second->ip_port())) {
      close(iter->first);
      delete iter->second;
      iter = conns_.erase(iter);
      deleting_conn_ipport_.erase(iter->second->ip_port());
      continue;
    }
    }
    // KillAllConns
    if (deleting_conn_ipport_.count(kKillAllConnsTask)) {
      Cleanup();
      return;
    }
    // Check keepalive timeout connection
    if (keepalive_timeout_ > 0 &&
        now.tv_sec - iter->second->last_interaction().tv_sec > keepalive_timeout_) {
      close(iter->first);
      delete iter->second;
      iter = conns_.erase(iter);
      continue;
    }
    ++iter;
  }
}

// clean all conns
void HolyThread::Cleanup() {
  slash::WriteLock l(&rwlock_);
  for (auto& iter : conns_) {
    close(iter.first);
    delete iter.second;
  }
  conns_.clear();
}

void HolyThread::KillAllConns() {
  KillConn(kKillAllConnsTask);
}

void HolyThread::KillConn(const std::string& ip_port) {
  bool find = false;
  {
  slash::ReadLock l(&rwlock_);
  for (auto& iter : conns_) {
    if (iter.second->ip_port() == ip_port) {
      find = true;
      break;
    }
  }
  }
  if (find || ip_port == kKillAllConnsTask) {
    slash::MutexLock l(&killer_mutex_);
    deleting_conn_ipport_.insert(ip_port);
  }
}

extern ServerThread *NewHolyThread(
    int port,
    ConnFactory *conn_factory,
    int cron_interval, const ServerHandle* handle) {
  return new HolyThread(port, conn_factory, cron_interval, handle);
}
extern ServerThread *NewHolyThread(
    const std::string &bind_ip, int port,
    ConnFactory *conn_factory,
    int cron_interval, const ServerHandle* handle) {
  return new HolyThread(bind_ip, port, conn_factory, cron_interval, handle);
}
extern ServerThread *NewHolyThread(
    const std::set<std::string>& bind_ips, int port,
    ConnFactory *conn_factory,
    int cron_interval, const ServerHandle* handle) {
  return new HolyThread(bind_ips, port, conn_factory, cron_interval, handle);
}

};  // namespace pink
