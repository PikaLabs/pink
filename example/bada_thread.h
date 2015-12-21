#ifndef BADA_THREAD_H_
#define BADA_THREAD_H_

#include "pb_thread.h"
#include "pink.pb.h"
#include "pb_conn.h"
#include <google/protobuf/message.h>

class BadaConn: public PbConn {
public:
  explicit BadaConn(int fd);
  virtual int DealMessage();
private:
  pink::Ping ping_;
  pink::PingRes pingRes_;
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
