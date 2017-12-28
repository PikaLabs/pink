// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_REDIS_CONN_H_
#define PINK_INCLUDE_REDIS_CONN_H_

#include <map>
#include <vector>
#include <string>

#include "slash/include/slash_status.h"
#include "pink/include/pink_define.h"
#include "pink/include/pink_conn.h"

namespace pink {

typedef std::vector<std::string> RedisCmdArgsType;

class RedisConn: public PinkConn {
 public:
  RedisConn(const int fd, const std::string &ip_port, ServerThread *thread);
  virtual ~RedisConn();

  virtual ReadStatus GetRequest();
  virtual WriteStatus SendReply();
  virtual void WriteResp(const std::string& resp);

  void TryResizeBuffer() override;

  virtual int DealMessage(RedisCmdArgsType& argv, std::string* response) = 0;

 private:
  ReadStatus ProcessInputBuffer();
  ReadStatus ProcessMultibulkBuffer();
  ReadStatus ProcessInlineBuffer();
  int FindNextSeparators();
  int GetNextNum(int pos, long *value);

  char* rbuf_;
  int rbuf_len_;
  int msg_peak_;
  RedisCmdArgsType argv_;

  uint32_t wbuf_pos_;
  std::string response_;

  // For Redis Protocol parser
  int last_read_pos_;
  int next_parse_pos_;
  int req_type_;
  long multibulk_len_;
  long bulk_len_;
};

}  // namespace pink
#endif  // PINK_INCLUDE_REDIS_CONN_H_
