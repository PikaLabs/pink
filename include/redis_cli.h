#ifndef PINK_REDIS_CLI_H_
#define PINK_REDIS_CLI_H_

#include <vector>
#include <string>

#include "pink_cli.h"

namespace pink {

typedef std::vector<std::string> RedisCmdArgsType;

class RedisCli : public PinkCli {
 public:
  RedisCli();
  ~RedisCli();

  //void Init(int send_timeout = 0, int recv_timeout = 0, int connect_timeout = 0);

  virtual Status Send(void *msg);
  virtual Status Recv(void *result);

 private:

  char *rbuf_;
  int32_t rbuf_len_;
  int32_t rbuf_pos_;

//  char *wbuf_;
//  int32_t wbuf_len_;
//  int32_t wbuf_pos_;

  int GetReply();
  int ProcessLineItem();

  RedisCli(const RedisCli&);
  void operator=(const RedisCli&);

 protected:
  RedisCmdArgsType argv_;
};

}

#endif
