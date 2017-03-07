// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <google/protobuf/message.h>

#include "include/pink_cli.h"
#include "include/pink_define.h"
#include "include/xdebug.h"

namespace pink {

// Default PBCli is block IO;
class PbCli : public PinkCli {
 public:
  PbCli();
  virtual ~PbCli();

  // msg should have been parsed
  virtual Status Send(void *msg_req) override;

  // Read, parse and store the reply
  virtual Status Recv(void *msg_res) override;


 private:
  // BuildWbuf need to access rbuf_, wbuf_;
  char *rbuf_;
  char *wbuf_;

  PbCli(const PbCli&);
  void operator= (const PbCli&);
};

PbCli::PbCli() {
  rbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * kProtoMaxMessage));
  wbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * kProtoMaxMessage));
}

PbCli::~PbCli() {
  free(wbuf_);
  free(rbuf_);
}

Status PbCli::Send(void *msg) {
  google::protobuf::Message *req = reinterpret_cast<google::protobuf::Message *>(msg);

  int wbuf_len = req->ByteSize();
  req->SerializeToArray(wbuf_ + kCommandHeaderLength, wbuf_len);
  uint32_t len = htonl(wbuf_len);
  memcpy(wbuf_, &len, sizeof(uint32_t));
  wbuf_len += kCommandHeaderLength;

  return PinkCli::SendRaw(wbuf_, wbuf_len);
}

Status PbCli::Recv(void *msg_res) {
  google::protobuf::Message *res = reinterpret_cast<google::protobuf::Message *>(msg_res);

  // Read Header
  size_t read_len = kCommandHeaderLength;
  Status s = RecvRaw((void *)rbuf_, &read_len);
  if (!s.ok()) {
    return s;
  }

  uint32_t integer;
  memcpy((char *)(&integer), rbuf_, sizeof(uint32_t));
  size_t packet_len = ntohl(integer);

  // Read Packet
  s = RecvRaw((void *)rbuf_, &packet_len);
  if (!s.ok()) {
    return s;
  }

  res->ParseFromArray(rbuf_, packet_len);
  return Status::OK();
}

extern PinkCli *NewPbCli() {
  return new PbCli();
}

};
