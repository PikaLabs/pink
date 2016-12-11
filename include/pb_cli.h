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
/*
 * pbcli support block client
 * I think block client is enough for a client
 */
class PbCli : public PinkCli {
 public:
  PbCli();
  virtual ~PbCli();

  virtual Status Send(void *msg) override;
  virtual Status Recv(void *msg_res) override;

 protected:
  virtual void BuildWbuf();

  int ReadHeader();
  int ReadPacket();

  int packet_len_;
  char *rbuf_;
  int32_t rbuf_len_;
  int32_t rbuf_pos_;

  char *wbuf_; /* Write buffer */
  int32_t wbuf_len_;
  int32_t wbuf_pos_;

  char *scratch_;
  google::protobuf::Message *msg_;
  google::protobuf::Message *msg_res_;

 private:
  PbCli(const PbCli&);
  void operator=(const PbCli&);
};

}  // namespace pink

#endif  // INCLUDE_PB_CLI_H_
