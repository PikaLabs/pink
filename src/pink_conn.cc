#include <stdio.h>
#include <unistd.h>

#include "pink_conn.h"

PinkConn::PinkConn(int fd):
  fd_(fd)
{
}

PinkConn::~PinkConn()
{
  close(fd_);
}
