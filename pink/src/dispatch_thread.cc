#include "pink/src/dispatch_thread.h"

#include "pink/src/pink_item.h"
#include "pink/src/pink_epoll.h"
#include "pink/src/worker_thread.h"

namespace pink {

DispatchThread::DispatchThread(int port,
                               int work_num, ConnFactory* conn_factory,
                               int cron_interval, const ServerHandle* handle,
                               const ThreadEnvHandle* ehandle)
      : ServerThread::ServerThread(port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num) {
  worker_thread_ = new WorkerThread*[work_num_];
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i] = new WorkerThread(conn_factory, cron_interval, ehandle);
  }
}

DispatchThread::DispatchThread(const std::string &ip, int port,
                               int work_num, ConnFactory* conn_factory,
                               int cron_interval, const ServerHandle* handle,
                               const ThreadEnvHandle* ehandle)
      : ServerThread::ServerThread(ip, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num) {
  worker_thread_ = new WorkerThread*[work_num_];
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i] = new WorkerThread(conn_factory, cron_interval, ehandle);
  }
}

DispatchThread::DispatchThread(const std::set<std::string>& ips, int port,
                               int work_num, ConnFactory* conn_factory,
                               int cron_interval, const ServerHandle* handle,
                               const ThreadEnvHandle* ehandle)
      : ServerThread::ServerThread(ips, port, cron_interval, handle),
        last_thread_(0),
        work_num_(work_num) {
  worker_thread_ = new WorkerThread*[work_num_];
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i] = new WorkerThread(conn_factory, cron_interval, ehandle);
  }
}

DispatchThread::~DispatchThread() {
  for (int i = 0; i < work_num_; i++) {
    delete worker_thread_[i];
  }
  delete worker_thread_;
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

void DispatchThread::HandleNewConn(const int connfd, const std::string& ip_port) {
  std::queue<PinkItem> *q = &(worker_thread_[last_thread_]->conn_queue_);
  PinkItem ti(connfd, ip_port);
  {
    slash::MutexLock l(&worker_thread_[last_thread_]->mutex_);
    q->push(ti);
  }
  write(worker_thread_[last_thread_]->notify_send_fd(), "", 1);
  last_thread_++;
  last_thread_ %= work_num_;
}

extern ServerThread *NewDispatchThread(
    int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval, const ServerHandle* handle,
    const ThreadEnvHandle* ehandle) {
  return new DispatchThread(port, work_num, conn_factory,
                            cron_interval, handle, ehandle);
}
extern ServerThread *NewDispatchThread(
    const std::string &ip, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval, const ServerHandle* handle,
    const ThreadEnvHandle* ehandle) {
  return new DispatchThread(ip, port, work_num, conn_factory,
                            cron_interval, handle, ehandle);
}
extern ServerThread *NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, ConnFactory* conn_factory,
    int cron_interval, const ServerHandle* handle,
    const ThreadEnvHandle* ehandle) {
  return new DispatchThread(ips, port, work_num, conn_factory,
                            cron_interval, handle, ehandle);
}

};  // namespace pink
