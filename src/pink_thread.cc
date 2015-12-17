#include "pink_thread.h"

  Thread::Thread()
: thread_id_(0)
{
}

Thread::~Thread()
{
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
