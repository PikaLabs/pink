#ifndef PINK_CONN_H_
#define PINK_CONN_H_

#include <sys/time.h>

#include "pink_define.h"

namespace pink {
class PinkConn
{
public:
  PinkConn(const int fd, const std::string &ip_port);
  virtual ~PinkConn();

  virtual ReadStatus GetRequest() = 0;
  virtual WriteStatus SendReply() = 0;

  virtual int DealMessage() = 0;



  void set_fd(const int fd) { 
    fd_ = fd; 
  }
  int fd() const {
    return fd_;
  }

  std::string ip_port() const {
    return ip_port_;
  }

  void set_is_reply(const bool is_reply) {
    is_reply_ = is_reply;
  }

  bool is_reply() const {
    return is_reply_;
  }

  void PlusConnQuerynum() {
    conn_querynum_++;
  }

  void set_conn_querynum(const uint64_t n) {
    conn_querynum_ = n;
  }

  uint64_t conn_querynum() const {
    return conn_querynum_;
  }

  void set_last_interaction(const struct timeval &now) {
    last_interaction_ = now;
  }

  struct timeval last_interaction() const {
    return last_interaction_;
  };

private:
  
  int fd_;
  std::string ip_port_;
  bool is_reply_;
  uint64_t conn_querynum_;
  struct timeval last_interaction_;

  /*
   * No allowed copy and copy assign operator
   */
  PinkConn(const PinkConn&);
  void operator=(const PinkConn&);
};
}

#endif
