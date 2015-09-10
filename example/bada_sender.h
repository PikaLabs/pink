#ifndef BADA_THREAD_H_
#define BADA_THREAD_H_

#include "pb_thread.h"
#include "pink.pb.h"
#include <google/protobuf/message.h>
#include "period_thread.h"

class BadaSender : public PeriodThread
{
public:
  BadaSender();
  virtual ~BadaSender();

  virtual void PeriodMain() = 0;

private:

  pink::Ping ping_;

  pink::PingRes pingRes_;
  int bada_num_;

};

#endif
