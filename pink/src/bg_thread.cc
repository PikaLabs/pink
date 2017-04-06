// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "pink/include/bg_thread.h"
#include <sys/time.h>
#include "slash/include/xdebug.h"

namespace pink {

void BGThread::Schedule(void (*function)(void*), void* arg) { mu_.Lock();
  while (queue_.size() >= full_ && running()) {
    wsignal_.Wait();
  }
  if (queue_.size() < full_) {
    queue_.push(BGItem(function, arg));
    rsignal_.Signal();
  }
  mu_.Unlock();
}

void *BGThread::ThreadMain() {
  while (running()) {
    mu_.Lock();
    while (queue_.empty() && timer_queue_.empty() && running()) {
      rsignal_.Wait();
    }
    if (!running()) {
      mu_.Unlock();
      break;
    }
    if (!timer_queue_.empty()) {
      struct timeval now;
      gettimeofday(&now, NULL);

      TimerItem timer_item = timer_queue_.top();
      uint64_t unow = now.tv_sec * 1000000 + now.tv_usec;
      if (unow / 1000 >= timer_item.exec_time / 1000) {
        void (*function)(void*) = timer_item.function;
        void* arg = timer_item.arg;
        timer_queue_.pop();
        wsignal_.Signal();
        mu_.Unlock();
        (*function)(arg);
        continue;
      } else if (queue_.empty() && running()) {
        rsignal_.TimedWait(static_cast<uint32_t>((timer_item.exec_time - unow) / 1000));
        mu_.Unlock();
        continue;
      }
    }
    if (!queue_.empty()) {
      void (*function)(void*) = queue_.front().function;
      void* arg = queue_.front().arg;
      queue_.pop();
      wsignal_.Signal();
      mu_.Unlock();
      (*function)(arg);
    }
  }
  return NULL;
}

/*
 * timeout is in millionsecond
 */
void BGThread::DelaySchedule(uint64_t timeout, void (*function)(void *), void* arg) {
  /*
   * pthread_cond_timedwait api use absolute API
   * so we need gettimeofday + timeout
   */
  struct timeval now;
  gettimeofday(&now, NULL);
  uint64_t exec_time;
  exec_time = now.tv_sec * 1000000 + timeout * 1000 + now.tv_usec;

  mu_.Lock();
  timer_queue_.push(TimerItem(exec_time, function, arg));
  rsignal_.Signal();
  mu_.Unlock();
}

/*
 * timeout is in millionsecond
 */
Timer::Timer(uint64_t interval, void (*function)(void*), void* arg) :
  Thread::Thread(),
  signal_(&mu_) {
    /*
     * pthread_cond_timedwait api use absolute API
     * so we need gettimeofday + timeout
     */
    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t exec_time = now.tv_sec * 1000000 + interval * 1000 + now.tv_usec;

    task_ = new TimerItem(exec_time, function, arg);
    interval_ = interval;
  }

bool Timer::Start() {
  if(StartThread() != 0) {
    return false;
  }
  return true;
}

void Timer::Stop() {
  if (running()) {
    set_running(false);
    signal_.Signal();
    JoinThread();
  }
}

void Timer::Reset() {
  if (!running()) {
    Start();
  }
  struct timeval now;
  gettimeofday(&now, NULL);
  uint64_t exec_time = now.tv_sec * 1000000 + interval_ * 1000 + now.tv_usec;
  slash::MutexLock l(&mu_);
  task_->exec_time = exec_time;
  signal_.Signal();
}

uint64_t Timer::RemainTime() {
  struct timeval now;
  slash::MutexLock l(&mu_);
  gettimeofday(&now, NULL);
  return (task_->exec_time - now.tv_sec * 1000000 - now.tv_usec) / 1000;
}

void *Timer::ThreadMain() {
  while (running()) {
    mu_.Lock();
    while (task_ == NULL && running()) {
      signal_.Wait();
    }
    if (!running()) {
      mu_.Unlock();
      break;
    }
    
    // Process task
    struct timeval now;
    gettimeofday(&now, NULL);

    uint64_t unow = now.tv_sec * 1000000 + now.tv_usec;
    if (unow / 1000 >= task_->exec_time / 1000) {
      void (*function)(void*) = task_->function;
      void* arg = task_->arg;
      mu_.Unlock();
      (*function)(arg);
      Reset();
      continue;
    } else {
      signal_.TimedWait(static_cast<uint32_t>((task_->exec_time - unow) / 1000));
      mu_.Unlock();
      continue;
    }
  }
  return NULL;
}


}
