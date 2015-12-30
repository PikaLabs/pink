#ifndef PINK_PB_CONN_H_
#define PINK_PB_CONN_H_

#include "status.h"
#include "csapp.h"
#include "pink_define.h"
#include "pink_util.h"
#include "pink_conn.h"
#include "xdebug.h"
#include <google/protobuf/message.h>
#include <map>

class PbConn: public PinkConn
{
public:
  explicit PbConn(int fd);
  ~PbConn();
  /*
   * Set the fd to nonblock && set the flag_ the the fd flag
   */
  bool SetNonblock();
  void InitPara();

  void SetIsReply(bool is_reply);
  bool IsReply();

  ReadStatus GetRequest();
  WriteStatus SendReply();

  int flags() { 
    return flags_; 
  };

  virtual int DealMessage() = 0;


  /*
   * The Variable need by read the buf,
   * We allocate the memory when we start the server
   */
  uint32_t header_len_;
  char* rbuf_;
  uint32_t cur_pos_;
  uint32_t rbuf_len_;

  ConnStatus connStatus_;

  google::protobuf::Message *res_;

  char* wbuf_;
  uint32_t wbuf_len_;
  uint32_t wbuf_pos_;

private:

  int flags_;
  bool is_reply_;

  Status BuildObuf();
};

#endif
