#include "pika_holy_thread.h"
#include "xdebug.h"

#include <functional>
#include <string>


PikaConn::PikaConn(int fd, std::string ip_port, Thread *thread) :
  RedisConn(fd, ip_port) {
  pika_thread_ = reinterpret_cast<PikaHolyThread *>(thread);
}

PikaConn::~PikaConn() {
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



PikaHolyThread::PikaHolyThread(int port, int cron_interval):
  HolyThread::HolyThread(port, cron_interval) {
  pika_num_ = 10;
}

int PikaHolyThread::PrintNum() {
  log_info("PikaHolyThread num %d", pika_num_);
  return 0;
}

PikaHolyThread::~PikaHolyThread() {

}
void PikaHolyThread::CronHandle() {
  log_info("======PikaHolyThread Cron======");
}
