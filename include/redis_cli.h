// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#ifndef PINK_REDIS_CLI_H_
#define PINK_REDIS_CLI_H_

#include <vector>
#include <string>

#include "pink_cli.h"

namespace pink {

typedef std::vector<std::string> RedisCmdArgsType;

class RedisCli {
 public:
  RedisCli();
  virtual ~RedisCli();

  // We can serialize redis command by 2 ways:
  // 1. by variable argmuments;
  //    eg.  RedisCli::Serialize(cmd, "set %s %d", "key", 5);
  //        cmd will be set as the result string;
  // 2. by a string vector;
  //    eg.  RedisCli::Serialize(argv, cmd);
  //        also cmd will be set as the result string.
  static int SerializeCommand(std::string *cmd, const char *format, ...);
  static int SerializeCommand(RedisCmdArgsType argv, std::string *cmd);

  // msg should have been parsed
  virtual Status Send(void *msg);

  // Read, parse and store the reply
  virtual Status Recv(void *result = NULL);

  RedisCmdArgsType argv_;   // The parsed result 


  // Wrapper of PinkCli
  int fd() {
    return cli_->fd();
  }
  Status Connect(const std::string &peer_ip, const int peer_port, const std::string& bind_ip = "") {
    return cli_->Connect(peer_ip, peer_port, bind_ip);
  }
  Status Close() {
    return cli_->Close();
  }

  // Set timeout in miliseconds, default send and recv timeout is 0,
  // default connect timeout is 1000ms
  int set_send_timeout(int send_timeout) {
    return cli_->set_send_timeout(send_timeout);
  }
  int set_recv_timeout(int recv_timeout) {
    return cli_->set_recv_timeout(recv_timeout);
  }
  void set_connect_timeout(int connect_timeout) {
    cli_->set_connect_timeout(connect_timeout);
  }
  bool Available() {
    return cli_->Available();
  }

 private:

  PinkCli* cli_;

  char *rbuf_;
  int32_t rbuf_size_;
  int32_t rbuf_pos_;
  int32_t rbuf_offset_;
  int elements_;    // the elements number of this current reply
  int err_;

  int GetReply();
  int GetReplyFromReader();

  int ProcessLineItem();
  int ProcessBulkItem();
  int ProcessMultiBulkItem();

  ssize_t BufferRead();
  char* ReadBytes(unsigned int bytes);
  char* ReadLine(int *_len);

  RedisCli(const RedisCli&);
  void operator=(const RedisCli&);
};

}   // namespace pink
#endif
