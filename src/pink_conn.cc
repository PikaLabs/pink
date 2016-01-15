#include <stdio.h>
#include <unistd.h>

#include "pink_conn.h"
#include "pink_thread.h"

PinkConn::PinkConn(int fd, std::string ip_port):
  fd_(fd),
  ip_port_(ip_port),
  is_reply_(false),
  conn_querynum_(0)
{
  gettimeofday(&last_interaction_, NULL);
}

PinkConn::~PinkConn()
{
  close(fd_);
}
