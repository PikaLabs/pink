#ifndef PINK_THREAD_H_
#define PINK_THREAD_H_

#include <pthread.h>

namespace pink {

class Thread
{

public:
  Thread(int cron_interval = 0);
  virtual ~Thread();
  void StartThread();
  virtual void CronHandle();
  int cron_interval_;


private:
  pthread_t thread_id_;

  static void *RunThread(void *arg);
  virtual void *ThreadMain() = 0; 
  
};

}

#endif
