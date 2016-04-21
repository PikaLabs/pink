#include "bg_thread.h"
namespace pink {
void BGThread::Schedule(void (*function)(void*), void* arg) {
  pthread_mutex_lock(&mu_);
  queue_.push_back(BGItem(function, arg));
  pthread_cond_signal(&signal_);
  pthread_mutex_unlock(&mu_);
}

void *BGThread::ThreadMain() {
  while (!should_exit_) {
    pthread_mutex_lock(&mu_);
    while (queue_.empty() && !should_exit_) {
      pthread_cond_wait(&signal_, &mu_);
    }
    if (should_exit_) {
      pthread_mutex_unlock(&mu_);
      continue;
    }
    void (*function)(void*) = queue_.front().function;
    void* arg = queue_.front().arg;
    queue_.pop_front();
    pthread_mutex_unlock(&mu_);
    (*function)(arg);
  }
  return NULL;
}
}
