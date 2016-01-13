#ifndef PINK_CONN_H_
#define PINK_CONN_H_

#include "pink_define.h"

class PinkConn
{
public:
  explicit PinkConn(int fd, std::string ip_port);
  virtual ~PinkConn();

  virtual ReadStatus GetRequest() = 0;
  virtual WriteStatus SendReply() = 0;

  virtual int DealMessage() = 0;



  void set_fd(int fd) { 
    fd_ = fd; 
  }
  int fd() {
    return fd_;
  }

  std::string ip_port() {
    return ip_port_;
  }

  void set_is_reply(bool is_reply) {
    is_reply_ = is_reply;
  }
  bool is_reply() {
    return is_reply_;
  }
private:
  
  int fd_;
  std::string ip_port_;
  bool is_reply_;
};

#endif
