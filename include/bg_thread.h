// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_BG_THREAD_H_
#define INCLUDE_BG_THREAD_H_

#include <atomic>
#include <deque>

#include "include/pink_thread.h"

namespace pink {

class BGThread : public Thread {
 public:
  explicit BGThread(int full = 100000) :
    Thread::Thread(), full_(full), running_(false) {
      pthread_mutex_init(&mu_, NULL);
      pthread_cond_init(&rsignal_, NULL);
      pthread_cond_init(&wsignal_, NULL);
    }

  virtual ~BGThread() {
    Stop();
    pthread_cond_destroy(&rsignal_);
    pthread_cond_destroy(&wsignal_);
    pthread_mutex_destroy(&mu_);
  }
  bool is_running() {
    return running_;
  }

  void Stop() {
    should_exit_ = true;
    if (running_) {
      pthread_cond_signal(&rsignal_);
      pthread_cond_signal(&wsignal_);
      pthread_join(thread_id(), NULL);
      running_ = false;
    }
  }

  void StartIfNeed() {
    bool expect = false;
    if (!running_.compare_exchange_strong(expect, true)) {
      return;
    }
    StartThread();
  }

  void Schedule(void (*function)(void*), void* arg);

 private:
  struct BGItem {
    void (*function)(void*);
    void* arg;
    BGItem(void (*_function)(void*), void* _arg)
      : function(_function), arg(_arg) {}
  };
  typedef std::deque<BGItem> BGQueue;
  pthread_mutex_t mu_;
  pthread_cond_t rsignal_;
  pthread_cond_t wsignal_;
  BGQueue queue_;
  size_t full_;
  // std::atomic<bool> exit_;
  std::atomic<bool> running_;
  virtual void *ThreadMain();
};

}  // namespace pink

#endif  // INCLUDE_BG_THREAD_H_
