// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pink_cli.h"
#include "include/status.h"
#include "include/pink_cli_socket.h"

namespace pink {

PinkCli::PinkCli(): available_(false) {
  cli_socket_ = new CliSocket();
}

PinkCli::~PinkCli() {
  delete cli_socket_;
}

inline int PinkCli::fd() {
  return cli_socket_->sockfd();
}

Status PinkCli::Close() {
  if (available_) {
    cli_socket_->Close();
    available_ = false;
  }
  return Status::OK();
}

Status PinkCli::Connect(const std::string &peer_ip, const int peer_port,
    const std::string &bind_ip) {
  Status s = cli_socket_->Connect(peer_ip, peer_port, bind_ip);
  available_ = s.ok() ? true : false;
  return s;
}

};  // namespace pink
