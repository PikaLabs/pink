#include <stdio.h>
#include <unistd.h>

#include "pink_conn.h"
#include "pink_util.h"
#include "pink_thread.h"

namespace pink {

PinkConn::PinkConn(const int fd, const std::string &ip_port):
  fd_(fd),
  ip_port_(ip_port),
  is_reply_(false)
{
  gettimeofday(&last_interaction_, NULL);
}

PinkConn::~PinkConn()
{
//  close(fd_);
}

bool PinkConn::SetNonblock()
{
  flags_ = Setnonblocking(fd());
  if (flags_ == -1) {
    return false;
  }
  return true;
}

}
