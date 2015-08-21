#ifndef PINK_MUTEXLOCK_H_
#define PINK_MUTEXLOCK_H_

#include <pthread.h>

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

#endif
