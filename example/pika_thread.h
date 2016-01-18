#ifndef PIKA_THREAD_H_
#define PIKA_THREAD_H_

#include "worker_thread.h"
#include "redis_conn.h"

using namespace pink;
class PikaThread;

class PikaConn: public RedisConn {
public:
  PikaConn(int fd, std::string ip_port, Thread *thread);
  virtual ~PikaConn();
  virtual int DealMessage();
private:
  PikaThread *pika_thread_;
};


class PikaThread : public WorkerThread<PikaConn>
{
public:
  PikaThread(int cron_interval = 0);
  virtual ~PikaThread();
  virtual void CronHandle();

  int PrintNum();

private:

  int pika_num_;

};

#endif
