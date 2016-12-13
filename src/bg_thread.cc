// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/bg_thread.h"

namespace pink {

void BGThread::Schedule(void (*function)(void*), void* arg) {
  mu_.Lock();
  while (queue_.size() >= full_ && running()) {
    wsignal_.Wait();
  }
  if (queue_.size() < full_) {
    queue_.push_back(BGItem(function, arg));
    rsignal_.Signal();
  }
  mu_.Unlock();
}

void *BGThread::ThreadMain() {
  while (running()) {
    mu_.Lock();
    while (queue_.empty() && running()) {
      rsignal_.Wait();
    }
    if (!running()) {
      mu_.Unlock();
      continue;
    }
    void (*function)(void*) = queue_.front().function;
    void* arg = queue_.front().arg;
    queue_.pop_front();
    wsignal_.Signal();
    mu_.Unlock();
    (*function)(arg);
  }
  return NULL;
}

}
