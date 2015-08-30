#include "bada_thread.h"
#include "xdebug.h"

#include <functional>
#include <string>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>



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

int BadaThread::DealMessage(const char* buf, const int len, google::protobuf::Message * &res)
{
  ping_.ParseFromArray(buf, len);
/*
 *   std::cout<<"Message is "<< ping_.DebugString();
 * 
 *   printf("%d %s %d\n", ping_.t(), ping_.address().data(), ping_.port());
 * 
 *   log_info("Bada Deal Message");
 */

  pingRes_.set_res(11234);
  pingRes_.set_mess("heiheidfdfdf");

  res = &pingRes_;

  int wbuf_len_ = res->ByteSize();

  return 0;
}

