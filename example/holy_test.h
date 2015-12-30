#ifndef HOLY_TEST_H_
#define HOLY_TEST_H_

#include "holy_thread.h"
#include "pink.pb.h"
#include "pb_conn.h"
#include <google/protobuf/message.h>


#include <vector>

class PinkThread;

class PinkHolyConn: public PbConn {
public:
  PinkHolyConn(int fd, Thread *thread);
  virtual ~PinkHolyConn();
  
  virtual int DealMessage();

private:
  pink::Ping ping_;
  pink::PingRes pingRes_;

  PinkThread *pink_thread_;
};


class PinkThread : public HolyThread<PinkHolyConn>
{
public:
  explicit PinkThread(int port);
  virtual ~PinkThread();

  std::vector<int> v;
  
private:


};

#endif
