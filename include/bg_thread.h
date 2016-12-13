// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_BG_THREAD_H_
#define PINK_INCLUDE_BG_THREAD_H_

#include <atomic>
#include <deque>

#include "include/pink_thread.h"
#include "third/slash/output/include/slash_mutex.h"

namespace pink {

class BGThread : public Thread {
 public:
  explicit BGThread(int full = 100000) :
    Thread::Thread(), 
    full_(full),
    mu_(),
    rsignal_(&mu_), 
    wsignal_(&mu_) {
    }

  virtual ~BGThread() {
    if (running()) {
      rsignal_.Signal();
      wsignal_.Signal();
      JoinThread();
      set_running(false);
    }
  }

  void Schedule(void (*function)(void*), void* arg);

  void TimeSchedule(void (*function)(void *), void* arg);

 private:

  struct BGItem {
    void (*function)(void*);
    void* arg;
    BGItem(void (*_function)(void*), void* _arg)
      : function(_function), arg(_arg) {}
  };

  typedef std::deque<BGItem> BGQueue;
  size_t full_;
  slash::Mutex mu_;
  slash::CondVar rsignal_;
  slash::CondVar wsignal_;
  BGQueue queue_;
  virtual void *ThreadMain();
};

}  // namespace pink

#endif  // PINK_INCLUDE_BG_THREAD_H_
