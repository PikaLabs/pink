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

  // We could set miliseconds timeout by 
  //    int set_send_timeout(int send_timeout)
  //    int set_recv_timeout(int recv_timeout)
  //    void set_connect_timeout(int connect_timeout)

  // We can serialize redis command by 2 ways:
  // 1. by variable argmuments;
  //    eg.  RedisCli::Serialize(cmd, "set %s %d", "key", 5);
  //        cmd will be set as the result string;
  // 2. by a string vector;
  //    eg.  RedisCli::Serialize(argv, cmd);
  //        also cmd will be set as the result string.
  static int SerializeCommand(std::string *cmd, const char *format, ...);
  static int SerializeCommand(RedisCmdArgsType argv, std::string *cmd);

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
  int elements_;    // the elements number of this current reply
  int err_;

//  char *wbuf_;
//  int32_t wbuf_len_;
//  int32_t wbuf_pos_;

  int GetReply();
  int GetReplyFromReader();

  int ProcessLineItem();
  int ProcessBulkItem();
  int ProcessMultiBulkItem();

  ssize_t BufferRead();
  char* ReadBytes(unsigned int bytes);
  char* ReadLine(int *_len);

  RedisCli(const RedisCli&);
  void operator=(const RedisCli&);

};

}   // namespace pink
#endif
