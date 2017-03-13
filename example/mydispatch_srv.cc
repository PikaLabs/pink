#include <stdio.h>
#include <unistd.h>

#include "include/pink_thread.h"
#include "include/server_thread.h"
#include "include/xdebug.h"

#include "myproto.pb.h"
#include "include/pb_conn.h"

#include <google/protobuf/message.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

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
  PbConn(fd, ip_port)
{
}

MyConn::~MyConn()
{
}

int MyConn::DealMessage()
{
  printf("In the myconn DealMessage branch\n");
  ping_.ParseFromArray(rbuf_ + cur_pos_ - header_len_, header_len_);
  ping_res_.Clear();
  ping_res_.set_res(11234);
  ping_res_.set_mess("heiheidfdfdf");
  printf ("DealMessage receive (%s)\n", ping_res_.mess().c_str());
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

int main()
{
  Thread *my_worker[10];
  ConnFactory *my_conn_factory = new MyConnFactory();
  for (int i = 0; i < 10; i++) {
    my_worker[i] = NewWorkerThread(my_conn_factory, 1000);
  }

  ServerThread *st = NewDispatchThread(9211, 10, my_worker, 1000);

  st->StartThread();
  st->JoinThread();

  return 0;
}
