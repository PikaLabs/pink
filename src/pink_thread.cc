#include "pink_thread.h"
#include "pink_thread_name.h"
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
  Thread* thread = reinterpret_cast<Thread*>(arg);
  if (!(thread->thread_name().empty()))
  {
    SetThreadName(pthread_self(), thread->thread_name());
    //bool result = SetThreadName(pthread_self(), thread->thread_name());
    //if (!result) {
    //  printf ("SetName failed\n");
    //} else {
    //  printf ("SetName OK\n");
    //}
  }
  thread->ThreadMain();
  return NULL;
}

}
