// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_SRC_CLI_SOCKET_H_
#define PINK_SRC_CLI_SOCKET_H_

#include <string>

#include "include/status.h"

namespace pink {

class CliSocket {
 public:
  CliSocket();
  virtual ~CliSocket();

  Status Connect(const std::string &peer_ip, const int peer_port, const std::string& bind_ip = "");
  Status Close();

  Status SendRaw(void *buf, size_t count);
  Status RecvRaw(void *buf, size_t* count);

  int fd() const;

  // Set timeout in miliseconds, default send and recv timeout is 0,
  // default connect timeout is 1000ms
  int set_send_timeout(int send_timeout);
  int set_recv_timeout(int recv_timeout);
  void set_connect_timeout(int connect_timeout);

  bool Available() {
    return available_;
  }

 private:
  bool available_;

  std::string peer_ip_;
  int peer_port_;

  int send_timeout_;
  int recv_timeout_;
  int connect_timeout_;

  bool keep_alive_;
  bool is_block_;

  int sockfd_;
};

};  // namespace pink

#endif  // PINK_SRC_CLI_SOCKET_H_
