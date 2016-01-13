#include "pika_thread.h"
#include "xdebug.h"

#include <functional>
#include <string>


PikaConn::PikaConn(int fd, std::string ip_port, Thread *thread) :
  RedisConn(fd, ip_port) {
  pika_thread_ = reinterpret_cast<PikaThread *>(thread);
}

int PikaConn::DealMessage() {

//  std::string res;
//  std::vector<std::string>::iterator iter = argv_.begin();
//  for (iter; iter != argv_.end(); iter++) {
//    res.append(*iter);
//    res.append(" ");
//  }
//  log_info("%s", res.c_str());
  memcpy(wbuf_ + wbuf_len_, "+OK\r\n", 5);
  wbuf_len_ += 5;
  return 0;
}



PikaThread::PikaThread(int cron_interval):
  WorkerThread::WorkerThread(cron_interval) {
  pika_num_ = 10;
}

int PikaThread::PrintNum() {
  log_info("PikaThread num %d", pika_num_);
  return 0;
}

PikaThread::~PikaThread() {

}
void PikaThread::CronHandle() {
  log_info("======PikaThread Cron======");
}
