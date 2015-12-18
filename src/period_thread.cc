#include "period_thread.h"
#include <unistd.h>

PeriodThread::PeriodThread(struct timeval period) :
  period_(period)
{
}

void *PeriodThread::ThreadMain()
{
  PeriodMain();
  select(0, NULL, NULL, NULL, &period_);
}
