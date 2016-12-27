// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_PB_CLI_H_
#define PINK_INCLUDE_PB_CLI_H_

#include <string>

#include <google/protobuf/message.h>

#include "src/cli_socket.h"

namespace pink {

// Default PBCli is block IO;
class PbCli {
 public:
  PbCli();
  virtual ~PbCli();

  virtual Status Send(void *msg_req);
  virtual Status Recv(void *msg_res);

  // Wrapper of PinkSocket
  int fd();
  Status Connect(const std::string &peer_ip, const int peer_port, const std::string& bind_ip = "");
  Status Close();

  // Set timeout in miliseconds, default send and recv timeout is 0,
  // default connect timeout is 1000ms
  int set_send_timeout(int send_timeout);
  int set_recv_timeout(int recv_timeout);
  void set_connect_timeout(int connect_timeout);
  bool Available() {
    return socket_->Available();
  }

 private:
  // BuildWbuf need to access rbuf_, wbuf_;
  char *rbuf_;
  char *wbuf_;

  CliSocket* socket_;

  PbCli(const PbCli&);
  void operator= (const PbCli&);
};

}  // namespace pink

#endif  // PINK_INCLUDE_PB_CLI_H_
