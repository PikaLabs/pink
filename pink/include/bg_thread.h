// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_BG_THREAD_H_
#define PINK_INCLUDE_BG_THREAD_H_

#include <atomic>
#include <queue>

#include "pink/include/pink_thread.h"

#include "slash/include/slash_mutex.h"

namespace pink {

struct TimerItem {
  uint64_t exec_time;
  void (*function)(void *);
  void* arg;
  TimerItem(uint64_t _exec_time, void (*_function)(void*), void* _arg) :
    exec_time(_exec_time),
    function(_function),
    arg(_arg) {}
  bool operator < (const TimerItem& item) const {
    return exec_time > item.exec_time;
  }
}; 
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
      set_running(false);
      rsignal_.Signal();
      wsignal_.Signal();
    }
  }

  void Schedule(void (*function)(void*), void* arg);

  /*
   * timeout is in millionsecond
   */
  void DelaySchedule(uint64_t timeout, void (*function)(void *), void* arg);

 private:

  struct BGItem {
    void (*function)(void*);
    void* arg;
    BGItem(void (*_function)(void*), void* _arg)
      : function(_function), arg(_arg) {}
  };


  std::queue<BGItem> queue_;
  std::priority_queue<TimerItem> timer_queue_;

  size_t full_;
  slash::Mutex mu_;
  slash::CondVar rsignal_;
  slash::CondVar wsignal_;
  virtual void *ThreadMain() override;
};

class Timer : public Thread {
 public:
  Timer(uint64_t interval, void (*function)(void*), void* arg,
      uint64_t interval_float = 0); 
  virtual ~Timer() {
    Stop();
    delete task_;
  }
  bool Start();
  void Stop();
  void Reset();
  // remian time in micosecond
  uint64_t RemainTime();

 private:
  uint64_t interval_;
  uint64_t interval_float_;
  TimerItem *task_;
  slash::Mutex mu_;
  slash::CondVar signal_;
  virtual void *ThreadMain() override;
};

}  // namespace pink

#endif  // PINK_INCLUDE_BG_THREAD_H_
