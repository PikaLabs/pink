#ifndef PIKA_HOLY_THREAD_H_
#define PIKA_HOLY_THREAD_H_

#include "holy_thread.h"
#include "redis_conn.h"

class PikaHolyThread;

class PikaConn: public RedisConn {
public:
  PikaConn(int fd, std::string ip_port, Thread *thread);
  virtual ~PikaConn();
  virtual int DealMessage();
private:
  PikaHolyThread *pika_thread_;
};


class PikaHolyThread : public HolyThread<PikaConn>
{
public:
  PikaHolyThread(int port, int cron_interval = 0);
  virtual ~PikaHolyThread();
  virtual void CronHandle();

  int PrintNum();

private:

  int pika_num_;

};

#endif
