#include "holy_test.h"
#include "xdebug.h"

#include "holy_thread.h"
#include <functional>
#include <string>
#include "pink.pb.h"
#include "pb_conn.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

using namespace pink;

PinkHolyConn::PinkHolyConn(int fd, std::string ip_port, Thread *thread) :
  PbConn(fd, ip_port)
{
  
  pink_thread_ = reinterpret_cast<PinkThread *>(thread);
  set_is_reply(false);

}

PinkHolyConn::~PinkHolyConn()
{
  pink_thread_ = NULL;
}

int PinkHolyConn::DealMessage()
{

  log_info("In the PinkHolyConn DealMessage branch");
  ping_.ParseFromArray(rbuf_ + 4, header_len_);
  
  printf("%s %d\n", ping_.address().data(), ping_.port());

  pink_thread_->v.push_back(123);

  log_info("pink_thread vestor size %zu", pink_thread_->v.size());
  pingRes_.set_res(11234);
  pingRes_.set_mess("heiheidfdfdf");

  res_ = &pingRes_;

  int wbuf_len_ = res_->ByteSize();

  return 0;
}



PinkThread::PinkThread(int port, int cron_interval) :
  HolyThread<PinkHolyConn>(port, cron_interval)
{

}


PinkThread::~PinkThread()
{

}

void PinkThread::CronHandle() {
  log_info("======HolyThread Cron======");
}
