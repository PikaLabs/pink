#ifndef BADA_THREAD_H_
#define BADA_THREAD_H_

#include "worker_thread.h"
#include "pink.pb.h"
#include "pb_conn.h"
#include <google/protobuf/message.h>

class BadaThread;

class BadaConn: public PbConn {
public:
  BadaConn(int fd, Thread *thread);
  virtual ~BadaConn();
  virtual int DealMessage();

private:
  pink::Ping ping_;
  pink::PingRes pingRes_;

  BadaThread *bada_thread_;
};


class BadaThread : public WorkerThread<BadaConn>
{
public:
  BadaThread();
  virtual ~BadaThread();

  int PrintNum();

private:

  int bada_num_;

};

#endif
