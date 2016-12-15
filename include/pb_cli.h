// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_PB_CLI_H_
#define PINK_INCLUDE_PB_CLI_H_

#include <string>

#include <google/protobuf/message.h>

#include "include/pink_cli.h"

namespace pink {

// Default PBCli is block IO;
class PbCli : public PinkCli {
 public:
  PbCli();
  virtual ~PbCli();

  virtual Status Send(void *msg_req);
  virtual Status Recv(void *msg_res);

 private:
  // BuildWbuf need to access rbuf_, wbuf_;
  char *rbuf_;
  char *wbuf_;

  PbCli(const PbCli&);
  void operator= (const PbCli&);
};

}  // namespace pink

#endif  // INCLUDE_PB_CLI_H_
