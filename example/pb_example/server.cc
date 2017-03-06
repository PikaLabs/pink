#include <stdio.h>
#include <unistd.h>

#include "include/server_thread.h"
#include "include/pink_conn.h"
#include "include/pb_conn.h"
#include "include/pink_thread.h"
#include "message.pb.h"

using namespace pink;

class PingConn : public PbConn {
 public:
  PingConn(int fd, std::string ip_port, pink::Thread* pself_thread = NULL) : 
    PbConn(fd, ip_port) {
  }
  virtual ~PingConn() {}

  int DealMessage() {
    request_.ParseFromArray(rbuf_ + cur_pos_ - header_len_, header_len_);

    response_.Clear();
    response_.set_pong("hello " + request_.ping());

    printf ("DealMessage receive (%s)\n", request_.ping().c_str());
    res_ = &response_;

    set_is_reply(true);
  }

 private:
  Thread* self_thread_;

  Ping request_;
  Pong response_;
  //google::protobuf::Message request_;
  //google::protobuf::Message response_;

  PingConn(PingConn&);
  PingConn& operator=(PingConn&);
};


class PingConnFactory : public ConnFactory {
 public:
  virtual PinkConn *NewPinkConn(int connfd, const std::string &ip_port, Thread *thread) const {
    return new PingConn(connfd, ip_port, thread);
  }
};


int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf ("Usage: ./server port\n");
    exit(0);
  }

  int my_port = (argc > 1) ? atoi(argv[1]) : 8221;

  ConnFactory *conn_factory = new PingConnFactory();

  ServerThread* my_thread = NewHolyThread(my_port, conn_factory, 1000);
  my_thread->StartThread();
  my_thread->JoinThread();

  delete my_thread;

  return 0;
}
