#ifndef SERVER_THREAD_H_
#define SERVER_THREAD_H_

#include <set>

#include "include/pink_define.h"
#include "include/pink_thread.h"
#include "include/pink_socket.h"
#include "include/pink_epoll.h"
#include "include/pink_mutex.h"

namespace pink {

class ServerThread : public Thread {
 public:
  explicit ServerThread(int port, int cron_interval = 0) :
    cron_interval_(cron_interval),
    port_(port) {
    ips_.insert("0.0.0.0");
  }

  ServerThread(const std::string& bind_ip, int port, int cron_interval = 0) :
    cron_interval_(cron_interval),
    port_(port) {
    ips_.insert(bind_ip);
  }

  ServerThread(const std::set<std::string>& bind_ips, int port, int cron_interval = 0) :
    cron_interval_(cron_interval),
    port_(port) {
    ips_ = bind_ips;
  }

  virtual ~ServerThread() {
    should_exit_ = true;
    JoinThread();

    delete(pink_epoll_);
    for (std::vector<ServerSocket*>::iterator iter = server_sockets_.begin();
        iter != server_sockets_.end();
        ++iter) {
      delete *iter;
    }
  }
  virtual int InitHandle() {
    int ret = 0;
    ServerSocket* socket_p;
    pink_epoll_ = new PinkEpoll();
    if (ips_.find("0.0.0.0") != ips_.end()) {
      ips_.clear();
      ips_.insert("0.0.0.0");
    }
    for (std::set<std::string>::iterator iter = ips_.begin();
        iter != ips_.end();
        ++iter) {
      socket_p = new ServerSocket(port_);
      ret = socket_p->Listen(*iter);
      if (ret != kSuccess) {
        return ret;
      }
      pink_epoll_->PinkAddEvent(socket_p->sockfd(), EPOLLIN | EPOLLERR | EPOLLHUP);
      server_sockets_.push_back(socket_p);
      server_fds_.insert(socket_p->sockfd());
    }
    return kSuccess;
  
  }

  virtual void CronHandle() {
  }

  virtual bool AccessHandle(std::string& ip) {
    return true;
  }

  virtual void *ThreadMain() = 0;

 protected:
  int cron_interval_;
  /*
   * The tcp server port and address
   */

  std::vector<ServerSocket*> server_sockets_;
  std::set<int32_t> server_fds_;

  int port_;
  std::set<std::string> ips_;

  /*
   * The Epoll event handler
   */
  PinkEpoll *pink_epoll_;
};
} // namespace pink

#endif
