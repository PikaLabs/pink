// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "pink_thread.h"
#include "pink_thread_name.h"
#include "xdebug.h"
#include "pink_define.h"

namespace pink {

Thread::Thread(int cron_interval)
  : cron_interval_(cron_interval),
    running_(false),
    thread_id_(0)
{
}

Thread::~Thread()
{
}

void Thread::CronHandle() {
}

int Thread::StartThread()
{
  bool expect = false;
  if (!running_.compare_exchange_strong(expect, true)) {
    return -1;
  }
  int ret = InitHandle();
  if (ret != kSuccess) {
    return ret;
  }
  ret = pthread_create(&thread_id_, NULL, RunThread, (void *)this);
  if (ret != 0) {
    return kCreateThreadError;
  }
  return kSuccess;
}

int Thread::JoinThread()
{
  if (thread_id_ != 0) {
    return pthread_join(thread_id_, NULL);
  }
  return -1;
}

void *Thread::RunThread(void *arg)
{
  Thread* thread = reinterpret_cast<Thread*>(arg);
  if (!(thread->thread_name().empty()))
  {
    SetThreadName(pthread_self(), thread->thread_name());
  }
  thread->ThreadMain();
  return NULL;
}

int Thread::InitHandle() {
  return kSuccess;
}

}
