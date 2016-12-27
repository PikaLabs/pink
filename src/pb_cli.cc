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
  socket_ = new CliSocket();
}

PbCli::~PbCli() {
  delete socket_;
  free(wbuf_);
  free(rbuf_);
}

// Wrapper of PbCli
int PbCli::fd() {
  return socket_->fd();
}
Status PbCli::Connect(const std::string &ip, const int port, const std::string &bind_ip) {
  return socket_->Connect(ip, port, bind_ip);
}
Status PbCli::Close() {
  return socket_->Close();
}
void PbCli::set_connect_timeout(int connect_timeout) {
  socket_->set_connect_timeout(connect_timeout);
}
int PbCli::set_send_timeout(int send_timeout) {
  return socket_->set_send_timeout(send_timeout);
}
int PbCli::set_recv_timeout(int recv_timeout) {
  return socket_->set_recv_timeout(recv_timeout);
}

Status PbCli::Send(void *msg) {
  if (!socket_->Available()) {
    return Status::IOError("unavailable connection");
  }

  google::protobuf::Message *req = reinterpret_cast<google::protobuf::Message *>(msg);

  int wbuf_len = req->ByteSize();
  req->SerializeToArray(wbuf_ + kCommandHeaderLength, wbuf_len);
  uint32_t len = htonl(wbuf_len);
  memcpy(wbuf_, &len, sizeof(uint32_t));
  wbuf_len += kCommandHeaderLength;

  return socket_->SendRaw(wbuf_, wbuf_len);
}

Status PbCli::Recv(void *msg_res) {
  if (!socket_->Available()) {
    return Status::IOError("unavailable connection");
  }

  google::protobuf::Message *res = reinterpret_cast<google::protobuf::Message *>(msg_res);

  // Read Header
  size_t read_len = kCommandHeaderLength;
  Status s = socket_->RecvRaw((void *)rbuf_, &read_len);
  if (!s.ok()) {
    return s;
  }

  uint32_t integer;
  memcpy((char *)(&integer), rbuf_, sizeof(uint32_t));
  size_t packet_len = ntohl(integer);

  // Read Packet
  s = socket_->RecvRaw((void *)rbuf_, &packet_len);
  if (!s.ok()) {
    return s;
  }

  res->ParseFromArray(rbuf_, packet_len);
  return Status::OK();
}

};
