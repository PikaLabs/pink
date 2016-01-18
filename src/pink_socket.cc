#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <event.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>

#include "pink_socket.h"
#include "pink_util.h"

namespace pink {

ServerSocket::ServerSocket(int port) :
  port_(port),
  send_timeout_(0),
  recv_timeout_(0),
  accept_timeout_(0),
  accept_backlog_(1024),
  tcp_send_buffer_(0),
  tcp_recv_buffer_(0),
  keep_alive_(false),
  listening_(false),
  is_block_(false) {
  }

ServerSocket::ServerSocket(int port, bool is_block) :
  port_(port),
  send_timeout_(0),
  recv_timeout_(0),
  accept_timeout_(0),
  accept_backlog_(1024),
  tcp_send_buffer_(0),
  tcp_recv_buffer_(0),
  keep_alive_(false),
  listening_(false),
  is_block_(is_block) {
  }

ServerSocket::~ServerSocket()
{
  Close();
}

void ServerSocket::Listen()
{
  sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
  memset(&servaddr_, 0, sizeof(servaddr_));

  int yes = 1;
  if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
  }

  servaddr_.sin_family = AF_INET;
  servaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr_.sin_port = htons(port_);

  bind(sockfd_, (struct sockaddr *) &servaddr_, sizeof(servaddr_));
  listen(sockfd_, accept_backlog_);
  listening_ = true;

  if (is_block_ == false) {
    SetNonBlock();
  }

}

int ServerSocket::SetNonBlock()
{
  flags_ = Setnonblocking(sockfd());
  if (flags_ == -1) {
    return -1;
  }
  return 0;
}

void ServerSocket::Close()
{
  close(sockfd_);
}
}
