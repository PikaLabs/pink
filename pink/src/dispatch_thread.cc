#include "pink/src/dispatch_thread.h"

#include "pink/src/pink_item.h"
#include "pink/src/pink_epoll.h"
#include "pink/include/worker_thread.h"

namespace pink {

DispatchThread::DispatchThread(int port,
                               int work_num, ConnFactory* conn_factory,
                               int cron_interval, int queue_limit,
                               const ServerHandle* handle,
                               const ThreadEnvHandle* ehandle)
      : ServerThread::ServerThread(port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        owned_worker_thread_(true) {
  worker_thread_ = new WorkerThread*[work_num_];
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i] = new WorkerThread(conn_factory, cron_interval, ehandle);
  }
}

DispatchThread::DispatchThread(const std::string &ip, int port,
                               int work_num, ConnFactory* conn_factory,
                               int cron_interval, int queue_limit,
                               const ServerHandle* handle,
                               const ThreadEnvHandle* ehandle)
      : ServerThread::ServerThread(ip, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        owned_worker_thread_(true) {
  worker_thread_ = new WorkerThread*[work_num_];
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i] = new WorkerThread(conn_factory, cron_interval, ehandle);
  }
}

DispatchThread::DispatchThread(const std::set<std::string>& ips, int port,
                               int work_num, ConnFactory* conn_factory,
                               int cron_interval, int queue_limit,
                               const ServerHandle* handle,
                               const ThreadEnvHandle* ehandle)
      : ServerThread::ServerThread(ips, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        owned_worker_thread_(true) {
  worker_thread_ = new WorkerThread*[work_num_];
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i] = new WorkerThread(conn_factory, cron_interval, ehandle);
  }
}

DispatchThread::DispatchThread(int port,
                               int work_num, WorkerThread** worker_thread,
                               int cron_interval, int queue_limit,
                               const ServerHandle* handle)
      : ServerThread::ServerThread(port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        worker_thread_(worker_thread),
        owned_worker_thread_(false) {
}

DispatchThread::DispatchThread(const std::string &ip, int port,
                               int work_num, WorkerThread** worker_thread,
                               int cron_interval, int queue_limit,
                               const ServerHandle* handle)
      : ServerThread::ServerThread(ip, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        worker_thread_(worker_thread),
        owned_worker_thread_(false) {
}

DispatchThread::DispatchThread(const std::set<std::string>& ips, int port,
                               int work_num, WorkerThread** worker_thread,
                               int cron_interval, int queue_limit,
                               const ServerHandle* handle)
      : ServerThread::ServerThread(ips, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num),
        worker_thread_(worker_thread),
        owned_worker_thread_(false) {
}

DispatchThread::~DispatchThread() {
  if (owned_worker_thread_) {
    for (int i = 0; i < work_num_; i++) {
      delete worker_thread_[i];
    }
    delete[] worker_thread_;
  }
}

int DispatchThread::StartThread() {
  for (int i = 0; i < work_num_; i++) {
    int ret = worker_thread_[i]->StartThread();
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
    int ret = worker_thread_[i]->StopThread();
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

int DispatchThread::conn_num() {
  int num = 0;
  for (int i = 0; i < work_num_; ++i) {
    num += worker_thread_[i]->conn_num();
  }
  return num;
} 

void DispatchThread::Cleanup() {
  for (int i = 0; i < work_num_; ++i) {
    worker_thread_[i]->Cleanup();
  }
}

void DispatchThread::HandleNewConn(const int connfd, const std::string& ip_port) {
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
}

extern ServerThread *NewDispatchThread(
    int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval, int queue_limit,
    const ServerHandle* handle,
    const ThreadEnvHandle* ehandle) {
  return new DispatchThread(port, work_num, conn_factory,
                            cron_interval, queue_limit, handle, ehandle);
}
extern ServerThread *NewDispatchThread(
    const std::string &ip, int port,
    int work_num, int queue_limit,
    ConnFactory* conn_factory,
    int cron_interval, const ServerHandle* handle,
    const ThreadEnvHandle* ehandle) {
  return new DispatchThread(ip, port, work_num, conn_factory,
                            cron_interval, queue_limit, handle, ehandle);
}
extern ServerThread *NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, int queue_limit,
    ConnFactory* conn_factory,
    int cron_interval, const ServerHandle* handle,
    const ThreadEnvHandle* ehandle) {
  return new DispatchThread(ips, port, work_num, conn_factory,
                            cron_interval, queue_limit, handle, ehandle);
}

extern ServerThread* NewDispatchThread(
    int port,
    int work_num, WorkerThread** worker_thread,
    int cron_interval, int queue_limit,
    const ServerHandle* handle) {
  return new DispatchThread(port, work_num, worker_thread,
                            cron_interval, queue_limit, handle);
}
extern ServerThread* NewDispatchThread(
    const std::string &ip, int port,
    int work_num, WorkerThread** worker_thread,
    int cron_interval, int queue_limit,
    const ServerHandle* handle) {
  return new DispatchThread(ip, port, work_num, worker_thread,
                            cron_interval, queue_limit, handle);
}
extern ServerThread* NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, WorkerThread** worker_thread,
    int cron_interval, int queue_limit,
    const ServerHandle* handle) {
  return new DispatchThread(ips, port, work_num, worker_thread,
                            cron_interval, queue_limit, handle);
}

};  // namespace pink
