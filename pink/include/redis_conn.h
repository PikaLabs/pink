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
#include "slash/include/xdebug.h"
#include "pink/include/pink_define.h"
#include "pink/include/pink_conn.h"

namespace pink {

typedef std::vector<std::string> RedisCmdArgsType;

class RedisConn: public PinkConn {
 public:
  RedisConn(const int fd, const std::string &ip_port, ServerThread *thread);
  virtual ~RedisConn();
  void ResetClient();

  bool ExpandWbufTo(uint32_t new_size);
  uint32_t wbuf_size_;

  virtual ReadStatus GetRequest();
  virtual WriteStatus SendReply();


  ConnStatus connStatus_;

 protected:
  char* wbuf_;
  uint32_t wbuf_len_;
  RedisCmdArgsType argv_;
  virtual int DealMessage() = 0;

 private:
  ReadStatus ProcessInputBuffer();
  ReadStatus ProcessMultibulkBuffer();
  ReadStatus ProcessInlineBuffer();
  int32_t FindNextSeparators();
  int32_t GetNextNum(int32_t pos, int32_t *value);
  int32_t last_read_pos_;
  int32_t next_parse_pos_;
  int32_t req_type_;
  int32_t multibulk_len_;
  int32_t bulk_len_;
  bool is_find_sep_;
  bool is_overtake_;

  /*
   * The Variable need by read the buf,
   * We allocate the memory when we start the server
   */
  char* rbuf_;
  uint32_t wbuf_pos_;
};

}  // namespace pink
#endif  // PINK_INCLUDE_REDIS_CONN_H_
