// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pink_thread.h"
#include "pink_thread_name.h"
#include "slash/include/xdebug.h"
#include "include/pink_define.h"

namespace pink {

Thread::Thread()
  : running_(false),
    thread_id_(0) {
}

Thread::~Thread() {
  set_running(false);
  JoinThread();
}

void* Thread::RunThread(void *arg) {
  Thread* thread = reinterpret_cast<Thread*>(arg);
  if (!(thread->thread_name().empty())) {
    SetThreadName(pthread_self(), thread->thread_name());
  }
  thread->ThreadMain();
  return NULL;
}

int Thread::StartThread() {
  bool expect = false;
  if (!running_.compare_exchange_strong(expect, true)) {
    return -1;
  }
  return pthread_create(&thread_id_, NULL, RunThread, (void *)this);
}

int Thread::JoinThread() {
    return pthread_join(thread_id_, NULL);
}


}  // namespace pink
