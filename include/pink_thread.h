// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_THREAD_H_
#define PINK_THREAD_H_

#include <string>
#include <atomic>
#include <pthread.h>

namespace pink {

class Thread
{
 public:
  explicit Thread(int cron_interval = 0);
  virtual ~Thread();
  int StartThread();
  void JoinThread();
  virtual void CronHandle();
  int cron_interval_;

  std::atomic<bool> should_exit_;


  pthread_t thread_id() const {
    return thread_id_;
  }

  std::string thread_name() const {
    return thread_name_;
  }
  void set_thread_name(const std::string &name) {
    thread_name_ = name;
  }
 private:

  static void *RunThread(void *arg);
  virtual void *ThreadMain() = 0; 
  virtual int InitHandle();

  pthread_t thread_id_;
  std::string thread_name_;

  /*
   * No allowed copy and copy assign
   */
  
  Thread(const Thread&);
  void operator=(const Thread&);
};

}

#endif
