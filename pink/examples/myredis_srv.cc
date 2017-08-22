#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <atomic>
#include <map>

#include "pink/include/server_thread.h"
#include "pink/include/pink_conn.h"
#include "pink/include/redis_conn.h"
#include "pink/include/pink_thread.h"

using namespace pink;

std::map<std::string, std::string> db;


class MyConn: public RedisConn {
 public:
  MyConn(int fd, const std::string& ip_port, ServerThread *thread,
         void* worker_specific_data);
  virtual ~MyConn();

 protected:
  virtual int DealMessage();

 private:
};

MyConn::MyConn(int fd, const std::string& ip_port,
               ServerThread *thread, void* worker_specific_data)
    : RedisConn(fd, ip_port, thread) {
  // Handle worker_specific_data ...
}

MyConn::~MyConn() {
}

int MyConn::DealMessage() {
  printf("Get redis message ");
  for (int i = 0; i < argv_.size(); i++) {
    printf("%s ", argv_[i].c_str());
  }
  printf("\n");

  std::string val = "result";
  std::string res;
  // set command
  if (argv_.size() == 3) {
    res = "+OK\r\n";
    db[argv_[1]] = argv_[2];
    memcpy(wbuf_ + wbuf_len_, res.data(), res.size());
    wbuf_len_ += res.size();
  } else {
    std::map<std::string, std::string>::iterator iter = db.find(argv_[1]);
    if (iter != db.end()) {
      val = iter->second;
      memcpy(wbuf_ + wbuf_len_, "*1\r\n$", 5);
      wbuf_len_ += 5;
      std::string len = std::to_string(val.length());
      memcpy(wbuf_ + wbuf_len_, len.data(), len.size());
      wbuf_len_ += len.size();
      memcpy(wbuf_ + wbuf_len_, "\r\n", 2);
      wbuf_len_ += 2;
      memcpy(wbuf_ + wbuf_len_, val.data(), val.size());
      wbuf_len_ += val.size();
      memcpy(wbuf_ + wbuf_len_, "\r\n", 2);
      wbuf_len_ += 2;
    } else {
      res = "$-1\r\n";
      memcpy(wbuf_ + wbuf_len_, res.data(), res.size());
      wbuf_len_ += res.size();
    }
  }
  set_is_reply(true);
  return 0;
}

class MyConnFactory : public ConnFactory {
 public:
  virtual PinkConn *NewPinkConn(int connfd, const std::string &ip_port,
                                ServerThread *thread,
                                void* worker_specific_data) const {
    return new MyConn(connfd, ip_port, thread, worker_specific_data);
  }
};

static std::atomic<bool> running(false);

static void IntSigHandle(const int sig) {
  printf("Catch Signal %d, cleanup...\n", sig);
  running.store(false);
  printf("server Exit");
}

static void SignalSetup() {
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, &IntSigHandle);
  signal(SIGQUIT, &IntSigHandle);
  signal(SIGTERM, &IntSigHandle);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("server will listen to 6379\n");
  } else {
    printf("server will listen to %d\n", atoi(argv[1]));
  }
  int my_port = (argc > 1) ? atoi(argv[1]) : 6379;

  SignalSetup();

  ConnFactory *conn_factory = new MyConnFactory();

  ServerThread* my_thread = NewHolyThread(my_port, conn_factory);
  if (my_thread->StartThread() != 0) {
    printf("StartThread error happened!\n");
    exit(-1);
  }
  running.store(true);
  while (running.load()) {
    sleep(1);
  }
  my_thread->StopThread();

  delete my_thread;
  delete conn_factory;

  return 0;
}
