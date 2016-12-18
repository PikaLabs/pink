// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "src/pink_cli.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>

#include "include/status.h"
#include "include/xdebug.h"

namespace pink {

PinkCli::PinkCli()
    : available_(false),
      send_timeout_(0),
      recv_timeout_(0),
      connect_timeout_(1000),
      keep_alive_(0),
      is_block_(true) {
}

PinkCli::~PinkCli() {
}

int PinkCli::fd() {
  return sockfd_;
}

Status PinkCli::Close() {
  if (available_) {
    close(sockfd_);
    available_ = false;
  }
  return Status::OK();
}

void PinkCli::set_connect_timeout(int connect_timeout) {
  connect_timeout_ = connect_timeout;
}

int PinkCli::set_send_timeout(int send_timeout) {
  int ret = 0;
  if (send_timeout > 0) {
    send_timeout_ = send_timeout;
    struct timeval timeout = {send_timeout_ / 1000, (send_timeout_ % 1000) * 1000};
    ret = setsockopt(sockfd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  }
  return ret;
}

int PinkCli::set_recv_timeout(int recv_timeout) {
  int ret = 0;
  if (recv_timeout > 0) {
    recv_timeout_ = recv_timeout;
    struct timeval timeout = {recv_timeout_ / 1000, (recv_timeout_ % 1000) * 1000};
    ret = setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  }
  return ret;
}

Status PinkCli::Connect(const std::string &ip, const int port, const std::string &bind_ip) {
  Status s;
  int rv;

  char cport[6];
  struct addrinfo hints, *servinfo, *p;
  snprintf(cport, 6, "%d", port);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  // We do not handle IPv6
  if ((rv = getaddrinfo(ip.c_str(), cport, &hints, &servinfo)) != 0) {
    return Status::IOError("connect getaddrinfo error for ", ip);
  }
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      continue;
    }

    // bind if needed
    if (!bind_ip.empty()) {
      struct sockaddr_in localaddr;
      localaddr.sin_family = AF_INET;
      localaddr.sin_addr.s_addr = inet_addr(bind_ip.c_str());
      localaddr.sin_port = 0;  // Any local port will do
      bind(sockfd_, (struct sockaddr *)&localaddr, sizeof(localaddr));
    }


    int flags = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);

    if (connect(sockfd_, p->ai_addr, p->ai_addrlen) == -1) {
      if (errno == EHOSTUNREACH) {
        close(sockfd_);
        continue;
      } else if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
        struct pollfd wfd[1];
        
        wfd[0].fd = sockfd_;
        wfd[0].events = POLLOUT;

        int res;
        if ((res = poll(wfd, 1, connect_timeout_)) == -1) {
          close(sockfd_);
          freeaddrinfo(servinfo);
          return Status::IOError("EHOSTUNREACH", "connect poll error");
        } else if (res == 0) {
          close(sockfd_);
          freeaddrinfo(servinfo);
          return Status::Timeout("");
          //return Status::IOError("ETIMEDOUT", "connect host timeout");
        }
        int val = 0;
        socklen_t lon = sizeof(int);

        if (getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &val, &lon) == -1) {
          close(sockfd_);
          freeaddrinfo(servinfo);
          return Status::IOError("EHOSTUNREACH", "connect host getsockopt error");
        }

        if (val) {
          close(sockfd_);
          freeaddrinfo(servinfo);
          return Status::IOError("EHOSTUNREACH", "connect host error");
        }
      } else {
        close(sockfd_);
        freeaddrinfo(servinfo);
        return Status::IOError("EHOSTUNREACH", "The target host cannot be reached");
      }
    }

    struct sockaddr_in laddr;
    socklen_t llen = sizeof(laddr);
    getsockname(sockfd_, (struct sockaddr*) &laddr, &llen);
    std::string lip(inet_ntoa(laddr.sin_addr));
    int lport = ntohs(laddr.sin_port);
    if (ip == lip && port == lport) 
    {
      return Status::IOError("EHOSTUNREACH", "same ip port");
    }

    flags = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags & ~O_NONBLOCK);
    freeaddrinfo(servinfo);
    available_ = true;
    return s;
  }
  if (p == NULL) {
    s = Status::IOError(strerror(errno), "Can't create socket ");
    return s;
  }
  freeaddrinfo(servinfo);
  freeaddrinfo(p);
  return s;
}

Status PinkCli::SendRaw(void *buf, size_t count) {
  if (!Available()) {
    return Status::IOError("unavailable connection");
  }

  char* wbuf = (char *)buf;
  size_t nleft = count;
  int pos = 0;
  ssize_t nwritten;

  while (nleft > 0) {
    if ((nwritten = write(sockfd_, wbuf + pos, nleft)) <= 0) {
      if (errno == EINTR) {
        nwritten = 0;
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return Status::Timeout("Send timeout");
      } else {
        return Status::IOError("write error " + std::string(strerror(errno)));
      }
    }

    nleft -= nwritten;
    pos += nwritten;
  }

  return Status::OK();
}

Status PinkCli::RecvRaw(void *buf, size_t *count) {
  if (!Available()) {
    return Status::IOError("unavailable connection");
  }

  char* rbuf = (char *)buf;
  size_t nleft = *count;
  size_t pos = 0;
  ssize_t nread;

  while (nleft > 0) {
    if ((nread = read(sockfd_, rbuf + pos, nleft)) <= 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return Status::Timeout("Send timeout");
      } else {
        return Status::IOError("read error " + std::string(strerror(errno)));
      }
    }
    log_info("nread %d", nread);
    nleft -= nread;
    pos += nread;
  }

  *count = pos;
  return Status::OK();
};

} // namespace pink
