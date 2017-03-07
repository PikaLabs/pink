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
    res_ = &response_;

    set_is_reply(true);
  }

 private:
  Thread* self_thread_;

  Ping request_;
  Pong response_;

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
    printf ("Usage: ./server ip port\n");
    exit(0);
  }

  std::string ip(argv[1]);
  int port = atoi(argv[2]);

  Thread *my_worker[24];
  ConnFactory *my_conn_factory = new PingConnFactory();
  for (int i = 0; i < 24; i++) {
    my_worker[i] = NewWorkerThread(my_conn_factory, 1000);
  }

  int my_port = (argc > 1) ? atoi(argv[1]) : 8221;

  ConnFactory *conn_factory = new PingConnFactory();

  ServerThread *st = NewDispatchThread(ip, port, 24, my_worker, 1000);
  st->StartThread();
  st->JoinThread();

  delete st;

  return 0;
}
