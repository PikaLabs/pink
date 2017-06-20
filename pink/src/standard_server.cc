#include "pink/src/standard_server.h"

#include "pink/src/pink_item.h"
#include "pink/src/pink_epoll.h"
#include "pink/src/worker_thread.h"

namespace pink {

StandardServer::StandardServer(int port,
                               int work_num, ConnFactory* conn_factory,
                               int queue_limit, int cron_interval,
                               const ServerHandle* handle)
      : ServerThread::ServerThread(port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        queue_limit_(queue_limit),
        conn_factory_(conn_factory),
        keepalive_timeout_(kDefaultKeepAliveTime) {
  if (work_num) {
    worker_thread_ = new WorkerThread*[work_num_];
    for (int i = 0; i < work_num_; i++) {
      worker_thread_[i] = new WorkerThread(conn_factory, this, cron_interval);
    }
  } else {
    worker_thread_ = nullptr;
  }
}

StandardServer::StandardServer(const std::string &ip, int port,
                               int work_num, ConnFactory* conn_factory,
                               int queue_limit, int cron_interval,
                               const ServerHandle* handle)
      : ServerThread::ServerThread(ip, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        queue_limit_(queue_limit) {
  if (work_num) {
    worker_thread_ = new WorkerThread*[work_num_];
    for (int i = 0; i < work_num_; i++) {
      worker_thread_[i] = new WorkerThread(conn_factory, this, cron_interval);
    }
  } else {
    worker_thread_ = nullptr;
  }
}

StandardServer::StandardServer(const std::set<std::string>& ips, int port,
                               int work_num, ConnFactory* conn_factory,
                               int queue_limit, int cron_interval,
                               const ServerHandle* handle)
      : ServerThread::ServerThread(ips, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        queue_limit_(queue_limit) {
  if (work_num) {
    worker_thread_ = new WorkerThread*[work_num_];
    for (int i = 0; i < work_num_; i++) {
      worker_thread_[i] = new WorkerThread(conn_factory, this, cron_interval);
    }
  } else {
    worker_thread_ = nullptr;
  }
}

StandardServer::~StandardServer() {
  for (int i = 0; i < work_num_; i++) {
    delete worker_thread_[i];
  }
  delete[] worker_thread_;
}

int StandardServer::StartThread() {
  int ret = 0;
  if (work_num_) {
    for (int i = 0; i < work_num_; i++) {
      ret = handle_->CreateWorkerSpecificData(&(worker_thread_[i]->private_data_));
      if (ret != 0) {
        return ret;
      }

      ret = worker_thread_[i]->StartThread();
      if (ret != 0) {
        return ret;
      }
      if (!thread_name().empty()) {
        worker_thread_[i]->set_thread_name("WorkerThread");
      }
    }
  } else {
    ret = handle_->CreateWorkerSpecificData(&private_data_);
    if (ret != 0) {
      return ret;
    }
  }

  return ServerThread::StartThread();
}

int StandardServer::StopThread() {
  int ret = 0;
  if (work_num_) {
    for (int i = 0; i < work_num_; i++) {
      ret = worker_thread_[i]->StopThread();
      if (ret != 0) {
        return ret;
      }
      ret = handle_->DeleteWorkerSpecificData(worker_thread_[i]->private_data_);
      if (ret != 0) {
        return ret;
      }
    }
  } else {
    ret = handle_->DeleteWorkerSpecificData(private_data_);
    if (ret != 0) {
      return ret;
    }
  }

  return ServerThread::StopThread();
}

void StandardServer::set_keepalive_timeout(int timeout) {
  for (int i = 0; i < work_num_; ++i) {
    worker_thread_[i]->set_keepalive_timeout(timeout);
  } 

  keepalive_timeout_ = timeout;
}

int StandardServer::conn_num() {
  int num = 0;
  for (int i = 0; i < work_num_; ++i) {
    num += worker_thread_[i]->conn_num();
  }

  {
    slash::ReadLock l(&rwlock_);
    num += conns_.size();
  }
  return num;
}

void StandardServer::KillAllConns() {
  KillConn(kKillAllConnsTask);
}

void StandardServer::KillConn(const std::string& ip_port) {
  if (work_num_) {
    for (int i = 0; i < work_num_; ++i) {
      worker_thread_[i]->TryKillConn(ip_port);
    }
  } else {
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
}

// clean all conns
void StandardServer::Cleanup() {
  assert(work_num_ == 0);
  slash::WriteLock l(&rwlock_);
  for (auto& iter : conns_) {
    close(iter.first);
    delete iter.second;
  }
  conns_.clear();
}

void StandardServer::DoCronTask() {
  if (work_num_ || keepalive_timeout_ <= 0) {
    return;
  }
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

void StandardServer::HandleNewConn(const int connfd, const std::string& ip_port) {
  if (work_num_) {
    // Slow workers may consume many fds.
    // We simply loop to find next legal worker.
    PinkItem ti(connfd, ip_port);
    int next_thread = last_thread_;
    bool find = false;
    for (int cnt = 0; cnt < work_num_; cnt++) {
      {
      slash::MutexLock l(&worker_thread_[next_thread]->mutex_);
      std::queue<PinkItem> *q = &(worker_thread_[next_thread]->conn_queue_);
      if (q->size() < static_cast<size_t>(queue_limit_)) {
        q->push(ti);
        find = true;
        break;
      }
      }
      next_thread = (next_thread + 1) % work_num_;
    }

    if (find) {
      write(worker_thread_[next_thread]->notify_send_fd(), "", 1);
      last_thread_ = (next_thread + 1) % work_num_;
      log_info("find worker(%d), refresh the last_thread_ to %d", next_thread, last_thread_);
    } else {
      log_info("all workers are full, queue limit is %d", queue_limit_);
      // every worker is full 
      // TODO(anan) maybe add log
      close(connfd);
    }
  } else {
    PinkConn *tc = conn_factory_->NewPinkConn(connfd, ip_port, this, private_data_);
    tc->SetNonblock();
    {
      slash::WriteLock l(&rwlock_);
      conns_[connfd] = tc;
    }

    pink_epoll_->PinkAddEvent(connfd, EPOLLIN);
  }
}

void StandardServer::HandleConnEvent(PinkFiredEvent *pfe) {
  assert(work_num_ == 0);
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

extern ServerThread *NewStandardServer(
    int port,
    int work_num, ConnFactory* conn_factory,
    int queue_limit, int cron_interval,
    const ServerHandle* handle) { 
  return new StandardServer(port, work_num, conn_factory,
                            queue_limit, cron_interval, handle);
}
extern ServerThread *NewStandardServer(
    const std::string &ip, int port,
    int work_num, ConnFactory* conn_factory,
    int queue_limit, int cron_interval,
    const ServerHandle* handle) {
  return new StandardServer(ip, port, work_num, conn_factory,
                            queue_limit, cron_interval, handle);
}
extern ServerThread *NewStandardServer(
    const std::set<std::string>& ips, int port,
    int work_num, ConnFactory* conn_factory,
    int queue_limit, int cron_interval,
    const ServerHandle* handle) {
  return new StandardServer(ips, port, work_num, conn_factory,
                            queue_limit, cron_interval, handle);
}

};  // namespace pink
