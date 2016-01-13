#include <stdio.h>
#include <unistd.h>

#include "pink_conn.h"
#include "pink_thread.h"

PinkConn::PinkConn(int fd, std::string ip_port):
  fd_(fd),
  ip_port_(ip_port),
  is_reply_(false)
{
}

PinkConn::~PinkConn()
{
  close(fd_);
}
