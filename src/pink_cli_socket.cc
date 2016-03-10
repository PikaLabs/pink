#include "pink_cli_socket.h"

CliSocket::CliSocket()
: send_timeout_(0),
  recv_timeout_(0),
  connect_timeout_(1000),
  keep_alive(0),
  is_block(true)
{
}


int CliSocket::set_send_timeout(int send_timeout) {
  send_timeout_ = send_timeout;
  int ret = 0;
  if (send_timeout_ > 0) {
    ret = setsockopt(sockfd_, SOL_SOCKET, SO_SNDTIMEO, &timeout_, sizeof(timeout_));
  }
  return ret;
}

int CliSocketset_recv_timeout(int recv_timeout) {
  recv_timeout_ = recv_timeout;
  int ret = 0;
  if (recv_timeout_ > 0) {
    ret = setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_));
  }
  return ret;
}

Status CliSocket::Connect(const std::string &ip, const int port)
{
  Status s;
  int sockfd, rv;

  char cport[6];
  struct addrinfo hints, *servinfo, *p;
  snprintf(cport, 6, "%d", port);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(ip.c_str(), cport, &hints, &servinfo)) != 0) {
    hints.ai_family = AF_INET6;
    if ((rv = getaddrinfo(ip.c_str(), cport, &hints, &servinfo)) != 0) {
      s = Status::IOError("tcp_connect error for ", ip);
      return s;
    }
  }
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      continue;
    }

    /*
     * we support connect timeout
     */
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
          return Status::IOError("ETIMEDOUT", "connect host timeout");
        }
        int val = 0;
        socklen_t lon = sizeof(int);

        if (getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &val, &lon) == -1) {
          freeaddrinfo(servinfo);
          return Status::IOError("EHOSTUNREACH", "connect host getsockopt error");
        }

        if (val) {
          freeaddrinfo(servinfo);
          return Status::IOError("EHOSTUNREACH", "connect host error");
        }
      } else {
        close(sockfd_);
        freeaddrinfo(servinfo);
        return Status::IOError("EHOSTUNREACH", "The target host cannot be reached");
      }
    }

    flags = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags & ~O_NONBLOCK);
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
