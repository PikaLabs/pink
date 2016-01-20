#ifndef PINK_REDIS_CONN_H_
#define PINK_REDIS_CONN_H_

#include "status.h"
#include "csapp.h"
#include "pink_define.h"
#include "pink_util.h"
#include "pink_conn.h"
#include "xdebug.h"
#include <map>
#include <vector>

namespace pink {

class RedisConn: public PinkConn
{
public:
  RedisConn(const int fd, const std::string &ip_port);
  virtual ~RedisConn();
  /*
   * Set the fd to nonblock && set the flag_ the the fd flag
   */
  bool SetNonblock();
  void ResetClient();

  int flags() const {
    return flags_;
  };

  virtual ReadStatus GetRequest();
  virtual WriteStatus SendReply();

  virtual int DealMessage() = 0;


  /*
   * The Variable need by read the buf,
   * We allocate the memory when we start the server
   */
  char* rbuf_;
  std::vector<std::string> argv_;

  ConnStatus connStatus_;


  char* wbuf_;
  uint32_t wbuf_pos_;
  uint32_t wbuf_len_;

private:
  int flags_;

  int32_t last_read_pos_;
  int32_t next_parse_pos_;
  int32_t req_type_;
  int32_t multibulk_len_;
  int32_t bulk_len_;
  bool is_find_sep_;
  bool is_overtake_;

  ReadStatus ProcessInputBuffer();
  ReadStatus ProcessMultibulkBuffer();
  ReadStatus ProcessInlineBuffer();
  int32_t FindNextSeparators();
  int32_t GetNextNum(int32_t pos, int32_t *value);

};
}

#endif
