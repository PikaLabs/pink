// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_PB_CONN_H_
#define INCLUDE_PB_CONN_H_

#include <string>
#include <map>

#include <google/protobuf/message.h>

#include "slash/include/slash_status.h"
#include "include/pink_conn.h"
#include "include/pink_define.h"

namespace pink {

using slash::Status;

class PbConn: public PinkConn {
 public:
  PbConn(const int fd, const std::string &ip_port);
  virtual ~PbConn();

  ReadStatus GetRequest() override;
  WriteStatus SendReply() override;

  virtual int DealMessage() = 0;

  /*
   * The Variable need by read the buf,
   * We allocate the memory when we start the server
   */
  uint32_t header_len_;
  char* rbuf_;
  uint32_t cur_pos_;
  uint32_t rbuf_len_;
  int32_t remain_packet_len_;

  ConnStatus connStatus_;

  google::protobuf::Message *res_;

 private:


  char* wbuf_;
  uint32_t wbuf_len_;
  uint32_t wbuf_pos_;
  virtual Status BuildObuf();
};

}  // namespace pink

#endif  // INCLUDE_PB_CONN_H_
