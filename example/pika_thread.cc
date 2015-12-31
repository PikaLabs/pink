#include "pika_thread.h"
#include "xdebug.h"

#include <functional>
#include <string>


PikaConn::PikaConn(int fd, Thread *thread) :
  RedisConn(fd)
{
  pika_thread_ = reinterpret_cast<PikaThread *>(thread);
}

int PikaConn::DealMessage()
{

  log_info("para num: %d", argv_.size());
  return 0;
}



PikaThread::PikaThread()
{
  pika_num_ = 10;
}

int PikaThread::PrintNum()
{
  log_info("PikaThread num %d", pika_num_);
  return 0;
}

PikaThread::~PikaThread()
{

}
