// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_PB_CONN_H_
#define PINK_INCLUDE_PB_CONN_H_

#include <string>
#include <map>

#include "google/protobuf/message.h"
#include "slash/include/slash_status.h"
#include "pink/include/pink_conn.h"
#include "pink/include/pink_define.h"

namespace pink {

using slash::Status;

class PbConn: public PinkConn {
 public:
  PbConn(const int fd, const std::string &ip_port, ServerThread *thread);
  virtual ~PbConn();

  ReadStatus GetRequest() override;
  WriteStatus SendReply() override;

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

 protected:
  // NOTE: if this function return non 0, the the server will close this connection
  //
  // In the implementation of DealMessage, we should distinguish two types of error
  //
  // 1. protocol parsing error
  // 2. service logic error
  //
  // protocol parsing error means that we receive a message that is not 
  // a protobuf message that we know,
  // in this situation we should close this connection.
  // why we should close connection?
  // beacause if we parse protocol error, it means that the content in this 
  // connection can't not be parse, we can't recognize the next message.
  // The only thing we can do is close this connection.
  // in this condition the DealMessage should return -1;
  //
  //
  // the logic error means that we have receive the message, and the 
  // message is protobuf message that we define in proto file.
  // After receiving this message, we start execute our service logic.
  // the service logic error we should put it in res_, and return 0
  // since this is the service logic error, not the network error.
  // this connection we can use again.
  // 
  virtual int DealMessage() = 0;

 private:
  char* wbuf_;
  uint32_t wbuf_len_;
  uint32_t wbuf_pos_;
  virtual Status BuildObuf();
};

}  // namespace pink
#endif  // PINK_INCLUDE_PB_CONN_H_
