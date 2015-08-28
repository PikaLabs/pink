#ifndef BADA_THREAD_H_
#define BADA_THREAD_H_

#include "pb_thread.h"
#include "pink.pb.h"

class BadaThread : public PbThread
{
public:
  BadaThread();
  virtual ~BadaThread();

  int PrintNum();

  virtual int DealMessage(const char* buf, const int len);

private:

  pink::Ping ping_;
  int bada_num_;

};

#endif
