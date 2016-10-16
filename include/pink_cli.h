// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_PINK_CLI_H_
#define INCLUDE_PINK_CLI_H_

#include <string>

#include "include/pink_cli_socket.h"
#include "include/status.h"

namespace pink {

class CliSocket;

class PinkCli {
 public:
  PinkCli();
  virtual ~PinkCli();

  Status Connect(const std::string &peer_ip, const int peer_port,
      const std::string& bind_ip = "");
  Status Close();
  virtual Status Send(void *msg) = 0;
  virtual Status Recv(void *msg_res) = 0;

  int fd();

  virtual int set_send_timeout(int send_timeout) {
    return cli_socket_->set_send_timeout(send_timeout);
  }

  virtual int set_recv_timeout(int recv_timeout) {
    return cli_socket_->set_recv_timeout(recv_timeout);
  }

  virtual void set_connect_timeout(int connect_timeout) {
    cli_socket_->set_connect_timeout(connect_timeout);
  }

  bool Available() {
    return available_;
  }

 private:
  bool available_;

  std::string peer_ip_;
  int peer_port_;

  CliSocket *cli_socket_;
};

};  // namespace pink

#endif  // INCLUDE_PINK_CLI_H_
