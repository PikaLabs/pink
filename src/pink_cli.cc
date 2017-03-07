// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pink_cli.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>

namespace pink {

struct PinkCli::Rep {
  std::string peer_ip;
  int peer_port;
  int send_timeout;
  int recv_timeout;
  int connect_timeout;
  bool keep_alive;
  bool is_block;
  int sockfd;

  Rep() : send_timeout(0),
      recv_timeout(0),
      connect_timeout(1000),
      keep_alive(0),
      is_block(true) {
      };
};

PinkCli::PinkCli()
  : rep_(new Rep()) {
}

PinkCli::~PinkCli() {
  Close();
  delete rep_;
}

Status PinkCli::Connect(const std::string &ip, const int port, 
    const std::string &bind_ip) {
  Rep* r = rep_;
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
    if ((r->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      continue;
    }

    // bind if needed
    if (!bind_ip.empty()) {
      struct sockaddr_in localaddr;
      localaddr.sin_family = AF_INET;
      localaddr.sin_addr.s_addr = inet_addr(bind_ip.c_str());
      localaddr.sin_port = 0;  // Any local port will do
      bind(r->sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr));
    }


    int flags = fcntl(r->sockfd, F_GETFL, 0);
    fcntl(r->sockfd, F_SETFL, flags | O_NONBLOCK);

    if (connect(r->sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      if (errno == EHOSTUNREACH) {
        close(r->sockfd);
        continue;
      } else if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
        struct pollfd wfd[1];
        
        wfd[0].fd = r->sockfd;
        wfd[0].events = POLLOUT;

        int res;
        if ((res = poll(wfd, 1, r->connect_timeout)) == -1) {
          close(r->sockfd);
          freeaddrinfo(servinfo);
          return Status::IOError("EHOSTUNREACH", "connect poll error");
        } else if (res == 0) {
          close(r->sockfd);
          freeaddrinfo(servinfo);
          return Status::Timeout("");
        }
        int val = 0;
        socklen_t lon = sizeof(int);

        if (getsockopt(r->sockfd, SOL_SOCKET, SO_ERROR, &val, &lon) == -1) {
          close(r->sockfd);
          freeaddrinfo(servinfo);
          return Status::IOError("EHOSTUNREACH", "connect host getsockopt error");
        }

        if (val) {
          close(r->sockfd);
          freeaddrinfo(servinfo);
          return Status::IOError("EHOSTUNREACH", "connect host error");
        }
      } else {
        close(r->sockfd);
        freeaddrinfo(servinfo);
        return Status::IOError("EHOSTUNREACH", "The target host cannot be reached");
      }
    }

    struct sockaddr_in laddr;
    socklen_t llen = sizeof(laddr);
    getsockname(r->sockfd, (struct sockaddr*) &laddr, &llen);
    std::string lip(inet_ntoa(laddr.sin_addr));
    int lport = ntohs(laddr.sin_port);
    if (ip == lip && port == lport) 
    {
      return Status::IOError("EHOSTUNREACH", "same ip port");
    }

    flags = fcntl(r->sockfd, F_GETFL, 0);
    fcntl(r->sockfd, F_SETFL, flags & ~O_NONBLOCK);
    freeaddrinfo(servinfo);
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
  char* wbuf = (char *)buf;
  size_t nleft = count;
  int pos = 0;
  ssize_t nwritten;

  while (nleft > 0) {
    if ((nwritten = write(rep_->sockfd, wbuf + pos, nleft)) <= 0) {
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
  Rep* r = rep_;
  char* rbuf = (char *)buf;
  size_t nleft = *count;
  size_t pos = 0;
  ssize_t nread;

  while (nleft > 0) {
    if ((nread = read(r->sockfd, rbuf + pos, nleft)) <= 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return Status::Timeout("Send timeout");
      } else {
        return Status::IOError("read error " + std::string(strerror(errno)));
      }
    }
    nleft -= nread;
    pos += nread;
  }

  *count = pos;
  return Status::OK();
};

int PinkCli::fd() const {
  return rep_->sockfd;
}

void PinkCli::Close() {
  close(rep_->sockfd);
}


void PinkCli::set_connect_timeout(int connect_timeout) {
  rep_->connect_timeout = connect_timeout;
}

int PinkCli::set_send_timeout(int send_timeout) {
  Rep* r = rep_;
  int ret = 0;
  if (send_timeout > 0) {
    r->send_timeout = send_timeout;
    struct timeval timeout = {r->send_timeout / 1000, (r->send_timeout % 1000) * 1000};
    ret = setsockopt(r->sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  }
  return ret;
}

int PinkCli::set_recv_timeout(int recv_timeout) {
  Rep* r = rep_;
  int ret = 0;
  if (recv_timeout > 0) {
    r->recv_timeout = recv_timeout;
    struct timeval timeout = {r->recv_timeout / 1000, (r->recv_timeout % 1000) * 1000};
    ret = setsockopt(r->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  }
  return ret;
}

};
