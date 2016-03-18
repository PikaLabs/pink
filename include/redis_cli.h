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

  int SerializeCommand(std::string *cmd, const char *format, ...);

  // msg should have been parsed
  virtual Status Send(void *msg);

  // Read, parse and store the reply
  virtual Status Recv(void *result = NULL);

  RedisCmdArgsType argv_;   // The parsed result 

 private:

  char *rbuf_;
  int32_t rbuf_size_;
  int32_t rbuf_pos_;
  int32_t rbuf_offset_;
  int err_;

//  char *wbuf_;
//  int32_t wbuf_len_;
//  int32_t wbuf_pos_;

  int GetReply();
  int GetReplyFromReader();

  int ProcessLineItem();
  ssize_t BufferRead();
  char* ReadBytes(unsigned int bytes);
  char* ReadLine(int *_len);

  RedisCli(const RedisCli&);
  void operator=(const RedisCli&);

};

}

#endif
