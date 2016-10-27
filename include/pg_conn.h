// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_PG_CONN_H_
#define INCLUDE_PG_CONN_H_

#include <string>
#include <map>

#include "pg_proto.h"
#include "csapp.h"
#include "pink_conn.h"
#include "pink_define.h"
#include "pink_util.h"
#include "xdebug.h"
#include "status.h"

namespace pink {

class PGConn: public PinkConn {
 public:
  PGConn(const int fd, const std::string &ip_port);
  ~PGConn();
  //void InitPara();

  ReadStatus GetRequest();
  WriteStatus SendReply();

  virtual int DealMessage() = 0;

  /*
   * The Variable need by read the buf,
   * We allocate the memory when we start the server
   */
  //uint32_t header_len_;
  char* rbuf_;
  uint32_t rbuf_size_;
  uint32_t rbuf_offset_;
  uint32_t parse_offset_;
  //int32_t remain_packet_len_;

  PGStatus conn_status_;

  char* wbuf_;
  uint32_t wbuf_size_;
  uint32_t wbuf_offset_;
  uint32_t write_offset_;

  std::string dbname_;
  std::string username_;
  std::string appname_;
  std::string client_encoding_;

  std::string statement_;
  InsertParser parser_;

 private:
  PacketHeader packet_header_;

  bool parse_error_;
  char cancel_key_[32];

  ReadStatus Recv(size_t count);
  //ReadStatus Recv(size_t count, ssize_t *nread);
  const char* ReadLine();
  void ResetRbuf();

  ReadStatus ClientProto();

  bool HandleStartupParameters();
  bool HandleStartup();
  ReadStatus HandleNormal();

  //virtual Status BuildObuf();
  virtual Status AppendWelcome();
  Status AppendSingleResponse(char type);
  Status AppendSpecialParameter();
  Status AppendReadyForQuery();
  Status AppendCommandComplete();
  Status AppendErrorResponse();

 protected:
  Status AppendObuf(const char* data, int size);
};

}  // namespace pink

#endif  // INCLUDE_PB_CONN_H_
