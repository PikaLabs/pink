#include "pink_thread.h"
#include "xdebug.h"

Thread::Thread(int cron_interval) :
  cron_interval_(cron_interval),
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
  pthread_create(&thread_id_, NULL, RunThread, (void *)this);
}

void *Thread::RunThread(void *arg)
{
  reinterpret_cast<Thread*>(arg)->ThreadMain();
  return NULL;
}
