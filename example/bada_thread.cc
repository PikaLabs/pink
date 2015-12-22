#include "bada_thread.h"
#include "xdebug.h"

#include "pb_conn.h"
#include <functional>
#include <string>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

BadaConn::BadaConn(int fd) :
  PbConn(fd)
{
}

int BadaConn::DealMessage()
{

  log_info("In the badaconn DealMessage branch");
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



BadaThread::BadaThread()
{
  bada_num_ = 10;

}

int BadaThread::PrintNum()
{
  log_info("BadaThread num %d", bada_num_);
  return 0;
}

BadaThread::~BadaThread()
{

}
