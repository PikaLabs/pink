#ifndef PINK_PB_CONN_H_
#define PINK_PB_CONN_H_

#include "status.h"
#include "csapp.h"
#include "pink_define.h"
#include "pink_util.h"
#include "pink_conn.h"
#include "xdebug.h"
#include <google/protobuf/message.h>
#include <string>
#include <map>

namespace pink {

class PbConn: public PinkConn
{
public:
  PbConn(const int fd, const std::string &ip_port);
  ~PbConn();
  void InitPara();


  ReadStatus GetRequest();
  WriteStatus SendReply();

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

  char* wbuf_;
  uint32_t wbuf_len_;
  uint32_t wbuf_pos_;

private:

  virtual Status BuildObuf();
};

}

#endif
