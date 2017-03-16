#include "src/dispatch_thread.h"

#include "src/pink_item.h"
#include "src/pink_epoll.h"
#include "src/worker_thread.h"

namespace pink {

DispatchThread::DispatchThread(int port,
    int work_num, Thread **worker_thread,
    int cron_interval) :
  ServerThread::ServerThread(port, cron_interval),
  last_thread_(0),
  work_num_(work_num),
  worker_thread_(reinterpret_cast<WorkerThread **>(worker_thread)) {
  StartWorkerThreads();
}

DispatchThread::DispatchThread(const std::string &ip, int port,
    int work_num, Thread **worker_thread,
    int cron_interval) :
  ServerThread::ServerThread(ip, port, cron_interval),
  last_thread_(0),
  work_num_(work_num),
  worker_thread_(reinterpret_cast<WorkerThread **>(worker_thread)) {
  StartWorkerThreads();
}

DispatchThread::DispatchThread(const std::set<std::string>& ips, int port,
    int work_num, Thread **worker_thread,
    int cron_interval) :
  ServerThread::ServerThread(ips, port, cron_interval),
  last_thread_(0),
  work_num_(work_num),
  worker_thread_(reinterpret_cast<WorkerThread **>(worker_thread)) {
  StartWorkerThreads();
}

void DispatchThread::HandleNewConn(const int connfd, const std::string& ip_port) {
  std::queue<PinkItem> *q = &(worker_thread_[last_thread_]->conn_queue_);
  PinkItem ti(connfd, ip_port);
  {
    MutexLock l(&worker_thread_[last_thread_]->mutex_);
    q->push(ti);
  }
  write(worker_thread_[last_thread_]->notify_send_fd(), "", 1);
  last_thread_++;
  last_thread_ %= work_num_;
}

void DispatchThread::StartWorkerThreads() {
  for (int i = 0; i < work_num_; i++) {
    worker_thread_[i]->StartThread();
  }
}

extern ServerThread *NewDispatchThread(
    int port,
    int work_num, Thread **worker_thread,
    int cron_interval) {
  return new DispatchThread(port, work_num, worker_thread, cron_interval);
}

extern ServerThread *NewDispatchThread(
    const std::string &ip, int port,
    int work_num, Thread **worker_thread,
    int cron_interval) {
  return new DispatchThread(ip, port, work_num, worker_thread, cron_interval);
}

extern ServerThread *NewDispatchThread(
    const std::set<std::string>& ips, int port,
    int work_num, Thread **worker_thread,
    int cron_interval) {
  return new DispatchThread(ips, port, work_num, worker_thread, cron_interval);
}


};  // namespace pink
