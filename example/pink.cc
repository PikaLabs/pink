#include <stdio.h>
#include <unistd.h>

#include "include/pink_thread.h"
#include "include/server_thread.h"
#include "include/xdebug.h"

#include "pink.pb.h"
#include "include/pb_conn.h"

#include <google/protobuf/message.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

using namespace pink;

class MyConn: public PbConn {
public:
  MyConn(int fd, std::string ip_port, Thread *thread);
  virtual ~MyConn();
  virtual int DealMessage();

private:
  pink::Ping ping_;
  pink::PingRes pingRes_;
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
  set_is_reply(true);
  log_info("In the badaconn DealMessage branch");
  ping_.ParseFromArray(rbuf_ + 4, header_len_);
  pingRes_.set_res(11234);
  pingRes_.set_mess("heiheidfdfdf");

  res_ = &pingRes_;

  int wbuf_len_ = res_->ByteSize();

  return 0;
}

class MyConnFactory : public ConnFactory {
 public:
  virtual PinkConn *NewPinkConn(int connfd, const std::string &ip_port, Thread *thread) const {
    return new MyConn(connfd, ip_port, thread);
  }
  
};

// class BadaThread
// {
// public:
//   BadaThread(int cron_interval = 0);
//   virtual ~BadaThread();

//   int PrintNum();

// private:
//   int bada_num_;
// };

// BadaThread::BadaThread(int cron_interval):
//   WorkerThread::WorkerThread(cron_interval) {
//   bada_num_ = 10;
// }

// int BadaThread::PrintNum() {
//   log_info("BadaThread num %d", bada_num_);
//   return 0;
// }

// BadaThread::~BadaThread() {

// }

int main()
{
  Thread *my_worker[10];
  ConnFactory *my_conn_factory = new MyConnFactory();
  for (int i = 0; i < 10; i++) {
    my_worker[i] = NewWorkerThread(my_conn_factory, 1000);
  }

  ServerThread *st = NewDispatchThread(9211, 10, my_worker, 1000);

  st->StartThread();


  sleep(1000);

  return 0;
}
