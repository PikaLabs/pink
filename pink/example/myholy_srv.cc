#include <stdio.h>
#include <unistd.h>

#include "pink/include/server_thread.h"
#include "pink/include/pink_conn.h"
#include "pink/include/pb_conn.h"
#include "pink/include/pink_thread.h"
#include "myproto.pb.h"

using namespace pink;

class MyConn: public PbConn {
 public:
  MyConn(int fd, std::string ip_port, Thread *thread);
  virtual ~MyConn();

 protected:
  virtual int DealMessage();

 private:
  myproto::Ping ping_;
  myproto::PingRes ping_res_;
};

MyConn::MyConn(int fd, ::std::string ip_port, Thread *thread) :
  PbConn(fd, ip_port, thread)
{
}

MyConn::~MyConn()
{
}

int MyConn::DealMessage()
{
  printf("In the myconn DealMessage branch\n");
  ping_.ParseFromArray(rbuf_ + cur_pos_ - header_len_, header_len_);
  printf ("DealMessage receive (%s) port %d \n", ping_.address().c_str(), ping_.port());

  ping_res_.Clear();
  ping_res_.set_res(11234);
  ping_res_.set_mess("heiheidfdfdf");
  res_ = &ping_res_;
  set_is_reply(true);
  return 0;
}

class MyConnFactory : public ConnFactory {
 public:
  virtual PinkConn *NewPinkConn(int connfd, const std::string &ip_port, Thread *thread) const {
    return new MyConn(connfd, ip_port, thread);
  }
};


int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf ("Usage: ./server port\n");
    exit(0);
  }

  int my_port = (argc > 1) ? atoi(argv[1]) : 8221;

  ConnFactory *conn_factory = new MyConnFactory();

  ServerThread* my_thread = NewHolyThread(my_port, conn_factory, 1000);
  my_thread->StartThread();
  my_thread->JoinThread();

  delete my_thread;
  delete conn_factory;

  return 0;
}
