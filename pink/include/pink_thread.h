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

class ThreadEnvHandle {
 public:
  ThreadEnvHandle() {}
  virtual ~ThreadEnvHandle() {}

  // Saved in thread's private_
  virtual int SetEnv(void** env) const = 0;
};

class Thread {
 public:
  Thread();
  virtual ~Thread();

  virtual int StartThread();
  virtual int StopThread();
  int JoinThread();

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

  void set_thread_name(const std::string& name) {
    thread_name_ = name;
  }

  void set_env_handle(const ThreadEnvHandle* ehandle) {
    ehandle_ = ehandle;
  }

  void* get_private() {
    return private_;
  }

 private:
  static void* RunThread(void* arg);
  virtual void *ThreadMain() = 0; 

  std::atomic<bool> running_;
  pthread_t thread_id_;
  std::string thread_name_;

  // User's private data
  const ThreadEnvHandle* ehandle_;
  void* private_;

  /*
   * No allowed copy and copy assign
   */
  Thread(const Thread&);
  void operator=(const Thread&);
};

}  // namespace pink

#endif  // INCLUDE_PINK_THREAD_H_
