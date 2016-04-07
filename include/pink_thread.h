#ifndef PINK_THREAD_H_
#define PINK_THREAD_H_

#include <atomic>
#include <pthread.h>

namespace pink {

class Thread
{
public:
  explicit Thread(int cron_interval = 0);
  virtual ~Thread();
  void StartThread();
  void JoinThread();
  virtual void CronHandle();
  int cron_interval_;

  std::atomic<bool> should_exit_;


  pthread_t thread_id() {
    return thread_id_;
  }
private:
  pthread_t thread_id_;

  static void *RunThread(void *arg);
  virtual void *ThreadMain() = 0; 

  /*
   * No allowed copy and copy assign
   */
  
  Thread(const Thread&);
  void operator=(const Thread&);
};

}

#endif
