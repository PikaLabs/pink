// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_PINK_THREAD_H_
#define INCLUDE_PINK_THREAD_H_

#include <string>
#include <atomic>
#include <pthread.h>

namespace pink {

class ConnFactory;

class Thread {
 public:
  Thread();
  virtual ~Thread();

  virtual int StartThread();
  int StopThread();

  bool running() const {
    return running_.load();
  }

  void set_running(bool running) {
    running_.store(running);
  }

  pthread_t thread_id() const {
    return thread_id_;
  }

  std::string thread_name() const {
    return thread_name_;
  }
  void set_thread_name(const std::string &name) {
    thread_name_ = name;
  }

 protected:
  int JoinThread();

 private:
  static void* RunThread(void* arg);
  virtual void *ThreadMain() = 0; 

  std::atomic<bool> running_;
  pthread_t thread_id_;
  std::string thread_name_;

  /*
   * No allowed copy and copy assign
   */
  Thread(const Thread&);
  void operator=(const Thread&);
};

extern Thread *NewWorkerThread(ConnFactory *conn_factory, int cron_interval = 0);

}  // namespace pink

#endif  // INCLUDE_PINK_THREAD_H_
