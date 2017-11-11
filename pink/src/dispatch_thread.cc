// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <vector>

#include "pink/src/dispatch_thread.h"

#include "pink/src/pink_item.h"
#include "pink/src/pink_epoll.h"
#include "pink/src/worker_thread.h"

namespace pink {

DispatchThread::DispatchThread(int port,
                               int work_num, ConnFactory* conn_factory,
                               int cron_interval, int queue_limit,
                               const ServerHandle* handle)
      : ServerThread::ServerThread(port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        queue_limit_(queue_limit) {
  worker_thread_ = new WorkerThread*[work_num_];
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i] = new WorkerThread(conn_factory, this, cron_interval);
  }
}

DispatchThread::DispatchThread(const std::string &ip, int port,
                               int work_num, ConnFactory* conn_factory,
                               int cron_interval, int queue_limit,
                               const ServerHandle* handle)
      : ServerThread::ServerThread(ip, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        queue_limit_(queue_limit) {
  worker_thread_ = new WorkerThread*[work_num_];
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i] = new WorkerThread(conn_factory, this, cron_interval);
  }
}

DispatchThread::DispatchThread(const std::set<std::string>& ips, int port,
                               int work_num, ConnFactory* conn_factory,
                               int cron_interval, int queue_limit,
                               const ServerHandle* handle)
      : ServerThread::ServerThread(ips, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        queue_limit_(queue_limit) {
  worker_thread_ = new WorkerThread*[work_num_];
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i] = new WorkerThread(conn_factory, this, cron_interval);
  }
}

DispatchThread::~DispatchThread() {
  for (int i = 0; i < work_num_; i++) {
    delete worker_thread_[i];
  }
  delete[] worker_thread_;
}

int DispatchThread::StartThread() {
  for (int i = 0; i < work_num_; i++) {
    int ret = handle_->CreateWorkerSpecificData(
        &(worker_thread_[i]->private_data_));
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
  return ServerThread::StartThread();
}

int DispatchThread::StopThread() {
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i]->set_should_stop();
  }
  for (int i = 0; i < work_num_; i++) {
    int ret = worker_thread_[i]->JoinThread();
    if (ret != 0) {
      return ret;
    }
    ret = handle_->DeleteWorkerSpecificData(worker_thread_[i]->private_data_);
    if (ret != 0) {
      return ret;
    }
  }
  return ServerThread::StopThread();
}

void DispatchThread::set_keepalive_timeout(int timeout) {
  for (int i = 0; i < work_num_; ++i) {
    worker_thread_[i]->set_keepalive_timeout(timeout);
  }
}

int DispatchThread::conn_num() const {
  int conn_num = 0;
  for (int i = 0; i < work_num_; ++i) {
    conn_num += worker_thread_[i]->conn_num();
  }
  return conn_num;
}

std::vector<ServerThread::ConnInfo> DispatchThread::conns_info() const {
  std::vector<ServerThread::ConnInfo> result;
  for (int i = 0; i < work_num_; ++i) {
    const auto worker_conns_info = worker_thread_[i]->conns_info();
    result.insert(
      result.end(),
      worker_conns_info.begin(),
      worker_conns_info.end());
  }
  return result;
}

PinkConn* DispatchThread::MoveConnOut(int fd) {
  for (int i = 0; i < work_num_; ++i) {
    PinkConn* conn = worker_thread_[i]->MoveConnOut(fd);
    if (conn != nullptr) {
      return conn;
    }
  }
  return nullptr;
}

bool DispatchThread::KillConn(const std::string& ip_port) {
  bool result = false;
  for (int i = 0; i < work_num_; ++i) {
    result = worker_thread_[i]->TryKillConn(ip_port) || result;
  }
  return result;
}

void DispatchThread::KillAllConns() {
  KillConn(kKillAllConnsTask);
}

void DispatchThread::HandleNewConn(
    const int connfd, const std::string& ip_port) {
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
    log_info("find worker(%d), refresh the last_thread_ to %d",
             next_thread, last_thread_);
  } else {
    log_info("all workers are full, queue limit is %d", queue_limit_);
    // every worker is full
    // TODO(anan) maybe add log
    close(connfd);
  }
}

extern ServerThread *NewDispatchThread(
    int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval, int queue_limit,
    const ServerHandle* handle) {
  return new DispatchThread(port, work_num, conn_factory,
                            cron_interval, queue_limit, handle);
}
extern ServerThread *NewDispatchThread(
    const std::string &ip, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval, int queue_limit,
    const ServerHandle* handle) {
  return new DispatchThread(ip, port, work_num, conn_factory,
                            cron_interval, queue_limit, handle);
}
extern ServerThread *NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval, int queue_limit,
    const ServerHandle* handle) {
  return new DispatchThread(ips, port, work_num, conn_factory,
                            cron_interval, queue_limit, handle);
}

};  // namespace pink
