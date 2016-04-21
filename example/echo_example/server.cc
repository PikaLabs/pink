#include <stdio.h>
#include <unistd.h>

#include "pink_thread.h"
#include "worker_thread.h"
#include "dispatch_thread.h"
#include "pb_conn.h"
#include "simple_message.pb.h"

class MyPbConn : public pink::PbConn {
public:
  MyPbConn(int fd, std::string ip_port, pink::Thread* self_thread_ptr = NULL) : pink::PbConn(fd, ip_port) {
    res_ = dynamic_cast<google::protobuf::Message*>(&message_);
  }
  ~MyPbConn() {}
  int DealMessage() {
    message_.ParseFromArray(rbuf_ + cur_pos_ - header_len_, header_len_);
    message_.set_name("hello " + message_.name());
    uint32_t u =htonl( message_.ByteSize());
    memcpy(static_cast<void*>(wbuf_), static_cast<void*>(&u), COMMAND_HEADER_LENGTH);
    message_.SerializeToArray(wbuf_ + COMMAND_HEADER_LENGTH, PB_MAX_MESSAGE);
    set_is_reply(true);
  }
 private:
  ::simple_message::SimpleMessage message_;
  MyPbConn(MyPbConn&);
  MyPbConn& operator=(MyPbConn&);
};

class MyWorkerThread : public pink::WorkerThread<MyPbConn> {
public:
  MyWorkerThread(int cron_interval = 0) : pink::WorkerThread<MyPbConn>(cron_interval) {}
  ~MyWorkerThread() {}
private:
  MyWorkerThread& operator=(MyWorkerThread&);
  MyWorkerThread(MyWorkerThread&);
};

class MyDispatchThread : public pink::DispatchThread<MyPbConn> {
public:
  MyDispatchThread(int my_port, int my_worker_threads_num, MyWorkerThread** my_worker_threads_ptr, int cron_interval) :
    pink::DispatchThread<MyPbConn>(my_port, my_worker_threads_num, reinterpret_cast<pink::WorkerThread<MyPbConn>**>(my_worker_threads_ptr), cron_interval) {}
  ~MyDispatchThread() {}
private:
  MyDispatchThread(MyDispatchThread&);
  MyDispatchThread& operator=(MyDispatchThread&);
};

int main(int argc, char* argv[]) {
  int my_port = (argc > 1) ? atoi(argv[1]) : 8221;
  int my_worker_threads_num = (argc > 2) ? atoi(argv[2]) : 1;
  int dispatch_cron_interval = 3000;//ms
  int worker_cron_interval = 1000;//ms
  MyWorkerThread** my_worker_threads_ptr = reinterpret_cast<MyWorkerThread**>(malloc(sizeof(MyWorkerThread*)*my_worker_threads_num));
  for (int i = 0; i != my_worker_threads_num; i++) {
    my_worker_threads_ptr[i] = new MyWorkerThread(worker_cron_interval);
  }
  MyDispatchThread *dispatch_thread = new MyDispatchThread(my_port, my_worker_threads_num, my_worker_threads_ptr, dispatch_cron_interval);
  dispatch_thread->StartThread();
  dispatch_thread->JoinThread();
  for (int i = 0; i != my_worker_threads_num; i++) {
    delete my_worker_threads_ptr[i];  
  }
  free(my_worker_threads_ptr);
}
