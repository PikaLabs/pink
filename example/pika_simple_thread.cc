#include "pika_simple_thread.h"
#include "xdebug.h"

#include <functional>
#include <string>

using namespace pink;

PikaSimpleThread::PikaSimpleThread(int num):
  SimpleThread::SimpleThread(),
  num_(num) {
}

PikaSimpleThread::~PikaSimpleThread() {

}

void* PikaSimpleThread::ThreadMain() {
  log_info("thread_id_: %lu", thread_id());
  log_info("num_: %d", num_);
  sleep(1);
  return NULL;
}
