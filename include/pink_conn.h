#ifndef PINK_CONN_H_
#define PINK_CONN_H_

#include <sys/time.h>

#include "pink_define.h"

namespace pink {
class PinkConn
{
public:
  PinkConn(int fd, std::string ip_port);
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

  void PlusConnQuerynum() {
    conn_querynum_++;
  }

  void set_conn_querynum(uint64_t n) {
    conn_querynum_ = n;
  }

  uint64_t conn_querynum() {
    return conn_querynum_;
  }

  void set_last_interaction(struct timeval* now) {
    last_interaction_ = *now;
  }

  struct timeval* last_interaction() {
    return &last_interaction_;
  };

private:
  
  int fd_;
  std::string ip_port_;
  bool is_reply_;
  uint64_t conn_querynum_;
  struct timeval last_interaction_;
};
}

#endif
