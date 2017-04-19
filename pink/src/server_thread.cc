// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "pink/include/server_thread.h"

#include "slash/include/xdebug.h"
#include "pink/src/pink_epoll.h"
#include "pink/src/server_socket.h"
#include "pink/src/csapp.h"

namespace pink {

using slash::Status;

class DefaultServerHandle : public ServerHandle {
public:
  virtual void CronHandle() const override {}
  virtual bool AccessHandle(std::string& ip) const override {
    return true;
  }
};

static const ServerHandle* SanitizeHandle(const ServerHandle* raw_handle) {
  if (raw_handle == nullptr) {
    return new DefaultServerHandle();
  }
  return raw_handle;
}

ServerThread::ServerThread(int port,
    int cron_interval, const ServerHandle* handle)
  : cron_interval_(cron_interval),
    handle_(SanitizeHandle(handle)),
    own_handle_(handle_ != handle),
    port_(port) {
  ips_.insert("0.0.0.0");
}

ServerThread::ServerThread(const std::string& bind_ip, int port,
    int cron_interval, const ServerHandle* handle)
  : cron_interval_(cron_interval),
    handle_(SanitizeHandle(handle)),
    own_handle_(handle_ != handle),
    port_(port) {
  ips_.insert(bind_ip);
}

ServerThread::ServerThread(const std::set<std::string>& bind_ips, int port,
    int cron_interval, const ServerHandle* handle)
  : cron_interval_(cron_interval),
    handle_(SanitizeHandle(handle)),
    own_handle_(handle_ != handle),
    port_(port) {
  ips_ = bind_ips;
}

ServerThread::~ServerThread() {
  delete(pink_epoll_);
  for (std::vector<ServerSocket*>::iterator iter = server_sockets_.begin();
       iter != server_sockets_.end();
       ++iter) {
    delete *iter;
  }
  if (own_handle_) {
    delete handle_;
  }
}

int ServerThread::StartThread() {
  int ret = 0;
  ret = InitHandle();
  if (ret != kSuccess)
    return ret;
  return Thread::StartThread();
}

int ServerThread::InitHandle() {
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

    // init pool
    pink_epoll_->PinkAddEvent(socket_p->sockfd(), EPOLLIN | EPOLLERR | EPOLLHUP);
    server_sockets_.push_back(socket_p);
    server_fds_.insert(socket_p->sockfd());
  }
  return kSuccess;
}

void *ServerThread::ThreadMain() {
  int nfds;
  PinkFiredEvent *pfe;
  Status s;
  struct sockaddr_in cliaddr;
  socklen_t clilen = sizeof(struct sockaddr);
  int fd, connfd;

  struct timeval when;
  gettimeofday(&when, nullptr);
  struct timeval now = when;

  when.tv_sec += (cron_interval_ / 1000);
  when.tv_usec += ((cron_interval_ % 1000) * 1000);
  int timeout = cron_interval_;
  if (timeout <= 0) {
    timeout = PINK_CRON_INTERVAL;
  }

  std::string ip_port;
  char port_buf[32];
  char ip_addr[INET_ADDRSTRLEN] = "";

  while (running()) {
    if (cron_interval_ > 0) {
      gettimeofday(&now, nullptr);
      if (when.tv_sec > now.tv_sec || (when.tv_sec == now.tv_sec && when.tv_usec > now.tv_usec)) {
        timeout = (when.tv_sec - now.tv_sec) * 1000 + (when.tv_usec - now.tv_usec) / 1000;
      } else {
        when.tv_sec = now.tv_sec + (cron_interval_ / 1000);
        when.tv_usec = now.tv_usec + ((cron_interval_ % 1000) * 1000);
        handle_->CronHandle();
        timeout = cron_interval_;
      }
    }

    nfds = pink_epoll_->PinkPoll(timeout);
    for (int i = 0; i < nfds; i++) {
      pfe = (pink_epoll_->firedevent()) + i;
      fd = pfe->fd;
      /*
       * Handle server event
       */
      if (server_fds_.find(fd) != server_fds_.end()) {
        if (pfe->mask & EPOLLIN) {
          connfd = accept(fd, (struct sockaddr *) &cliaddr, &clilen);
          if (connfd == -1) {
            log_warn("accept error, errno numberis %d, error reason %s", errno, strerror(errno));
            continue;
          }
          fcntl(connfd, F_SETFD, fcntl(connfd, F_GETFD) | FD_CLOEXEC);

          ip_port = inet_ntop(AF_INET, &cliaddr.sin_addr, ip_addr, sizeof(ip_addr));

          if (!handle_->AccessHandle(ip_port)) {
            close(connfd);
            continue;
          }

          ip_port.append(":");
          snprintf(port_buf, sizeof(port_buf), "%d", ntohs(cliaddr.sin_port));
          ip_port.append(port_buf);

          /*
           * Handle new connection,
           * implemented in derived class
           */
          HandleNewConn(connfd, ip_port);

        } else if (pfe->mask & (EPOLLHUP | EPOLLERR)) {
          /*
           * this branch means there is error on the listen fd
           */
          close(pfe->fd);
          continue;
        }
      } else {
        /*
         * Handle connection's event
         * implemented in derived class
         */
        HandleConnEvent(pfe);
      }
    }
  }
  return nullptr;
}

}  // namespace pink
