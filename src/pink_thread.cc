#include "pink_thread.h"
#include "xdebug.h"

namespace pink {

Thread::Thread(int cron_interval)
  : cron_interval_(cron_interval),
    should_exit_(false),
    thread_id_(0)
{
}

Thread::~Thread()
{
}

void Thread::CronHandle() {
//  log_info("Come in thread cronhandle");
}

void Thread::StartThread()
{
  should_exit_ = false;
  pthread_create(&thread_id_, NULL, RunThread, (void *)this);
}

void Thread::JoinThread()
{
  if (thread_id_ != 0)
    pthread_join(thread_id_, NULL);
}

void *Thread::RunThread(void *arg)
{
  reinterpret_cast<Thread*>(arg)->ThreadMain();
  return NULL;
}

}
