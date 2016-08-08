#ifndef BG_THREAD_H_
#define BG_THREAD_H_

#include <deque>
#include <atomic>
#include "pink_thread.h"

namespace pink {

class BGThread : public Thread {
  public:
    BGThread(int full = 100000) :
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
    //std::atomic<bool> exit_;
    std::atomic<bool> running_;
    virtual void *ThreadMain(); 
};

}

#endif
