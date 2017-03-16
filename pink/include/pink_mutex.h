// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#ifndef PINK_MUTEXLOCK_H_
#define PINK_MUTEXLOCK_H_

#include <pthread.h>

namespace pink {

class CondVar;

class Mutex {
public:
  Mutex();
  ~Mutex();

  void Lock();
  void Unlock();
  void AssertHeld() { }

private:
  friend class CondVar;
  pthread_mutex_t mu_;

  // No copying
  Mutex(const Mutex&);
  void operator=(const Mutex&);
};

class CondVar {
public:
  explicit CondVar(Mutex* mu);
  ~CondVar();
  void Wait();
  void Signal();
  void SignalAll();
private:
  pthread_cond_t cv_;
  Mutex* mu_;
};

class MutexLock {
public:
  explicit MutexLock(Mutex *mu)
    : mu_(mu)  {
      this->mu_->Lock();
    }
  ~MutexLock() { this->mu_->Unlock(); }

private:
  Mutex *const mu_;
  // No copying allowed
  MutexLock(const MutexLock&);
  void operator=(const MutexLock&);
};

typedef pthread_once_t OnceType;
extern void InitOnce(OnceType* once, void (*initializer)());

class RWLock {
public:
  RWLock(pthread_rwlock_t *mu, bool is_rwlock) :
  mu_(mu) {
    if (is_rwlock) {
      pthread_rwlock_wrlock(this->mu_);
    } else {
      pthread_rwlock_rdlock(this->mu_);
    }
  }
  ~RWLock() { pthread_rwlock_unlock(this->mu_); }

private:
  pthread_rwlock_t *const mu_;
  // No copying allowed
  RWLock(const RWLock&);
  void operator=(const RWLock&);
};

}

#endif
