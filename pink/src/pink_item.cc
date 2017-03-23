// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "pink_item.h"
#include "pink/include/pink_define.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include "csapp.h"

namespace pink {

PinkItem::PinkItem(const int fd, const std::string &ip_port) :
    fd_(fd),
    ip_port_(ip_port)
{
}

PinkItem::~PinkItem()
{
}

}
