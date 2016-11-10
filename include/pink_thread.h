#ifndef PINK_THREAD_H_
#define PINK_THREAD_H_

#include <string>
#include <atomic>
#include <pthread.h>

namespace pink {

class Thread
{
public:
  explicit Thread(int cron_interval = 0);
  virtual ~Thread();
  int StartThread();
  void JoinThread();
  virtual void CronHandle();
  int cron_interval_;

  std::atomic<bool> should_exit_;


  pthread_t thread_id() {
    return thread_id_;
  }

  const std::string thread_name() {
    return thread_name_;
  }
  void set_thread_name(const std::string &name) {
    thread_name_ = name;
  }
private:
  pthread_t thread_id_;
  std::string thread_name_;

  static void *RunThread(void *arg);
  virtual void *ThreadMain() = 0; 
  virtual int InitHandle();

  /*
   * No allowed copy and copy assign
   */
  
  Thread(const Thread&);
  void operator=(const Thread&);
};

}

#endif
