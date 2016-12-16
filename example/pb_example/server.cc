#include <stdio.h>
#include <unistd.h>

#include "holy_thread.h"
#include "pb_conn.h"
#include "message.pb.h"

using namespace pink;

class ServerThread;

class PingConn : public pink::PbConn {
 public:
  PingConn(int fd, std::string ip_port, pink::Thread* pself_thread = NULL) : PbConn(fd, ip_port) {
   // self_thread_ = dynamic_cast<ServerThread*>(pself_thread);
    //res_ = dynamic_cast<google::protobuf::Message*>(&message_);
  }
  ~PingConn() {}

  int DealMessage() {
    request_.ParseFromArray(rbuf_ + cur_pos_ - header_len_, header_len_);

    response_.Clear();
    response_.set_pong("hello " + request_.ping());

    printf ("DealMessage receive (%s)\n", request_.ping().c_str());
    res_ = &response_;

    set_is_reply(true);
  }

 private:
  ServerThread* self_thread_;

  Ping request_;
  Pong response_;
  //google::protobuf::Message request_;
  //google::protobuf::Message response_;

  PingConn(PingConn&);
  PingConn& operator=(PingConn&);
};

class ServerThread : public HolyThread<PingConn> {
 public:
  ServerThread(int port, int cron_interval = 0) : HolyThread(port, cron_interval) {
  }
  virtual ~ServerThread() {
  }
  virtual void CronHandle() {
  }

 private:
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf ("Usage: ./server port\n");
    exit(0);
  }

  int my_port = (argc > 1) ? atoi(argv[1]) : 8221;

  ServerThread* server_thread = new ServerThread(my_port, 1000);
  server_thread->StartThread();

  server_thread->JoinThread();

  delete server_thread;
  return 0;
}
