// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <stdio.h>
#include <unistd.h>

#include "pink_conn.h"
#include "pink_util.h"
#include "pink_thread.h"

namespace pink {

PinkConn::PinkConn(const int fd, const std::string &ip_port, Thread *thread):
  fd_(fd),
  ip_port_(ip_port),
  is_reply_(false),
  thread_(thread)
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
