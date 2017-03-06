// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_ITEM_H_
#define PINK_ITEM_H_

#include "slash/include/slash_status.h"

#include "pink_define.h"

namespace pink {
  
class PinkItem
{
public:
  PinkItem() {};
  PinkItem(const int fd, const std::string &ip_port);
  ~PinkItem();

  int fd() const {
    return fd_;
  }
  std::string ip_port() const {
    return ip_port_;
  }

private:

  int fd_;
  std::string ip_port_;

  /*
   * Here we should allow the copy and copy assign operator
   */
  // PinkItem(const PinkItem&);
  // void operator=(const PinkItem&);

};

}

#endif
