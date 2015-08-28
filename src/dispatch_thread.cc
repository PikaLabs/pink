#include "dispatch_thread.h"

#include "pink_thread.h"
#include "pink_util.h"
#include "pb_thread.h"

DispatchThread::DispatchThread(int port, int work_num, PbThread **pbThread) :
  port_(port),
  work_num_(work_num), 
  pbThread_(pbThread)
{
  sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
  memset(&servaddr_, 0, sizeof(servaddr_));

  servaddr_.sin_family = AF_INET;
  servaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr_.sin_port = htons(port_);

  bind(sockfd_, (struct sockaddr *) &servaddr_, sizeof(servaddr_));
  listen(sockfd_, 10);

  Setnonblocking(sockfd_);

  // init epoll
  pinkEpoll_ = new PinkEpoll();
  pinkEpoll_->PinkAddEvent(sockfd_, EPOLLIN | EPOLLERR | EPOLLHUP);


  last_thread_ = 0;
  for (int i = 0; i < work_num_; i++) {
    pbThread_[i]->StartThread();
  }
}


DispatchThread::~DispatchThread()
{
}

void *DispatchThread::ThreadMain()
{
  int nfds;
  PinkFiredEvent *pfe;
  Status s;
  struct sockaddr_in cliaddr;
  socklen_t clilen;
  int fd, connfd;
  for (;;) {
    nfds = pinkEpoll_->PinkPoll();
    pfe = pinkEpoll_->firedevent();
    for (int i = 0; i < nfds; i++) {
      fd = (pfe + i)->fd_;
      if (fd == sockfd_ && ((pfe + i)->mask_ & EPOLLIN)) {
        connfd = accept(sockfd_, (struct sockaddr *) &cliaddr, &clilen);
        log_info("Accept new fd %d", connfd);
        std::queue<PinkItem> *q = &(pbThread_[last_thread_]->conn_queue_);
        log_info("pfe must happen");
        PinkItem ti(connfd);
        {
          MutexLock l(&pbThread_[last_thread_]->mutex_);
          q->push(ti);
        }
        write(pbThread_[last_thread_]->notify_send_fd(), "", 1);
        last_thread_++;
        last_thread_ %= work_num_;
      } else if ((pfe + i)->mask_ & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
        log_info("Epoll timeout event");
      }
    }
  }
}
