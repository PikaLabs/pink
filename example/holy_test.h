#ifndef HOLY_TEST_H_
#define HOLY_TEST_H_

#include "holy_thread.h"
#include "pink.pb.h"
#include "pb_conn.h"
#include <google/protobuf/message.h>

class PinkHolyConn: public PbConn {
public:
  explicit PinkHolyConn(int fd);
  virtual int DealMessage();


private:
  pink::Ping ping_;
  pink::PingRes pingRes_;
};


class PinkThread : public HolyThread<PinkHolyConn>
{
public:
  explicit PinkThread(int port);
  virtual ~PinkThread();


private:


};

#endif
