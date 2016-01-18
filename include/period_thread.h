#ifndef PINK_PERIOD_THREAD_H_
#define PINK_PERIOD_THREAD_H_

#include "pink_thread.h"
#include <sys/time.h> 

namespace pink {

class PeriodThread : public Thread
{
public:
  PeriodThread(struct timeval period = (struct timeval){1, 0});
  virtual void *ThreadMain();
  virtual void PeriodMain() = 0;

private:
  struct timeval period_;

};
}

#endif
