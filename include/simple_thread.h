#ifndef SIMPLE_THREAD_H_
#define SIMPLE_THREAD_H_

#include "csapp.h"
#include "xdebug.h"
#include "pink_thread.h"
#include "pink_util.h"

namespace pink {
class SimpleThread : public Thread {
public:
  explicit SimpleThread() {
  };
  virtual ~SimpleThread() {
  };
private:
  // No copying allowed
  SimpleThread(const SimpleThread&);
  void operator=(const SimpleThread&);
};
}

#endif
