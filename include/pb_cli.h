#ifndef PINK_PB_CLI_H_
#define PINK_PB_CLI_H_

#include <string>
#include "pink_cli.h"
#include <google/protobuf/message.h>

namespace pink {
/*
 * pbcli support block client
 * I think block client is enough for a client
 */
class PbCli : public PinkCli
{
public:
  PbCli();
  ~PbCli();


  virtual Status Send(void *msg);
  virtual Status Recv(void *msg_res);

protected:

  virtual void BuildWbuf();

  int ReadHeader();
  int ReadPacket();

  
  int packet_len_;
  char *rbuf_;
  int32_t rbuf_len_;
  int32_t rbuf_pos_;

  char *wbuf_; /* Write buffer */
  int32_t wbuf_len_;
  int32_t wbuf_pos_;

  char *scratch_;
  google::protobuf::Message *msg_;
  google::protobuf::Message *msg_res_;

private:
  PbCli(const PbCli&);
  void operator=(const PbCli&);
};

}

#endif
