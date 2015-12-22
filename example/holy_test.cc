#include "holy_test.h"
#include "xdebug.h"

#include "holy_thread.h"
#include <functional>
#include <string>
#include "pink.pb.h"
#include "pb_conn.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

PinkHolyConn::PinkHolyConn(int fd) :
  PbConn(fd)
{

}

int PinkHolyConn::DealMessage()
{

  log_info("In the PinkHolyConn DealMessage branch");
  ping_.ParseFromArray(rbuf_ + 4, header_len_);
  /*
   *   std::cout<<"Message is "<< ping_.DebugString();
   * 
   *   printf("%d %s %d\n", ping_.t(), ping_.address().data(), ping_.port());
   * 
   *   log_info("Bada Deal Message");
   */

  pingRes_.set_res(11234);
  pingRes_.set_mess("heiheidfdfdf");

  res_ = &pingRes_;

  int wbuf_len_ = res_->ByteSize();

  return 0;
}



PinkThread::PinkThread(int port) :
  HolyThread<PinkHolyConn>(port)
{

}


PinkThread::~PinkThread()
{

}
