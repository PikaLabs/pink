// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/pb_cli.h"
#include "include/pink_define.h"
#include "include/xdebug.h"

namespace pink {

PbCli::PbCli() {
  rbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * kProtoMaxMessage));
  wbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * kProtoMaxMessage));
}

PbCli::~PbCli() {
  free(wbuf_);
  free(rbuf_);
}

Status PbCli::Send(void *msg) {
  if (!Available()) {
    return Status::IOError("unavailable connection");
  }

  log_info("The Send function");
  google::protobuf::Message *req = reinterpret_cast<google::protobuf::Message *>(msg);

  int wbuf_len = req->ByteSize();
  req->SerializeToArray(wbuf_ + kCommandHeaderLength, wbuf_len);
  uint32_t len = htonl(wbuf_len);
  memcpy(wbuf_, &len, sizeof(uint32_t));
  wbuf_len += kCommandHeaderLength;

  return PinkCli::SendRaw(wbuf_, wbuf_len);
}

Status PbCli::Recv(void *msg_res) {
  if (!Available()) {
    return Status::IOError("unavailable connection");
  }

  log_info("The Recv function");
  google::protobuf::Message *res = reinterpret_cast<google::protobuf::Message *>(msg_res);

  // Read Header
  size_t read_len = kCommandHeaderLength;
  Status s = PinkCli::RecvRaw((void *)rbuf_, &read_len);
  if (!s.ok()) {
    return s;
  }

  uint32_t integer;
  memcpy((char *)(&integer), rbuf_, sizeof(uint32_t));
  size_t packet_len = ntohl(integer);
  log_info("packet_len %d", packet_len);

  // Read Packet
  s = PinkCli::RecvRaw((void *)rbuf_, &packet_len);
  if (!s.ok()) {
    return s;
  }

  res->ParseFromArray(rbuf_, packet_len);
  return Status::OK();
}

};
