#ifndef PINK_CONN_H_
#define PINK_CONN_H_

#include "pink_define.h"

class PinkConn
{
public:
  explicit PinkConn(int fd);
  ~PinkConn();

  virtual ReadStatus GetRequest() = 0;
  virtual WriteStatus SendReply() = 0;

  // PinkConn *CreateConn(ConnFactory &ConnFactory);

  virtual int DealMessage() = 0;


  void set_fd(int fd) { 
    fd_ = fd; 
  }
  int fd() {
    return fd_;
  }
private:
  
  int fd_;
};

#endif
