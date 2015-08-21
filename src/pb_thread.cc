#include "pb_thread.h"
#include "pink_mutex.h"
#include "csapp.h"
#include "xdebug.h"
#include "pink_epoll.h"
#include <sys/epoll.h>
#include "pb_conn.h"

PbThread::PbThread()
{
    /*
     * install the protobuf handler here
     */

    pinkEpoll_ = new PinkEpoll();
    int fds[2];
    if (pipe(fds)) {
        // LOG(FATAL) << "Can't create notify pipe";
        log_err("Can't create notify pipe");
    }
    notify_receive_fd_ = fds[0];
    notify_send_fd_ = fds[1];
    pinkEpoll_->PinkAddEvent(notify_receive_fd_, EPOLLIN | EPOLLERR | EPOLLHUP);

}

PbThread::~PbThread()
{

}

/*
 * int PbThread::Install(std::string &name, handler &h)
 * {
 *     map[name] = h;
 * }
 * 
 */

void *PbThread::ThreadMain()
{
    int nfds;
    PinkFiredEvent *tfe = NULL;
    char bb[1];
    PinkItem ti;
    PbConn *inConn;
    for (;;) {
        nfds = pinkEpoll_->PinkPoll();
        /*
         * log_info("nfds %d", nfds);
         */
        for (int i = 0; i < nfds; i++) {
            tfe = (pinkEpoll_->firedevent()) + i;
            log_info("tfe->fd_ %d tfe->mask_ %d", tfe->fd_, tfe->mask_);
            if (tfe->fd_ == notify_receive_fd_ && (tfe->mask_ & EPOLLIN)) {
                read(notify_receive_fd_, bb, 1);
                {
                MutexLock l(&mutex_);
                ti = conn_queue_.front();
                conn_queue_.pop();
                }
                PbConn *tc = new PbConn(ti.fd());
                tc->SetNonblock();
                conns_[ti.fd()] = tc;

                pinkEpoll_->PinkAddEvent(ti.fd(), EPOLLIN);
                log_info("receive one fd %d", ti.fd());
                /*
                 * tc->set_thread(this);
                 */
            }
            int shouldClose = 0;
            if (tfe->mask_ & EPOLLIN) {
                inConn = conns_[tfe->fd_];
                log_info("come if readable %d", (inConn == NULL));
                if (inConn == NULL) {
                    continue;
                }
                if (inConn->PbGetRequest() == 0) {
                    pinkEpoll_->PinkModEvent(tfe->fd_, 0, EPOLLOUT);
                } else {
                    delete(inConn);
                    shouldClose = 1;
                }
            }
            log_info("tfe mask %d %d %d", tfe->mask_, EPOLLIN, EPOLLOUT);
            if (tfe->mask_ & EPOLLOUT) {
                log_info("Come in the EPOLLOUT branch");
                inConn = conns_[tfe->fd_];
                if (inConn == NULL) {
                    continue;
                }
                if (inConn->PbSendReply() == 0) {
                    log_info("SendReply ok");
                    pinkEpoll_->PinkModEvent(tfe->fd_, 0, EPOLLIN);
                }
            }
            if ((tfe->mask_  & EPOLLERR) || (tfe->mask_ & EPOLLHUP)) {
                log_info("close tfe fd here");
                close(tfe->fd_);
            }
            if (shouldClose) {
                log_info("close tfe fd here");
                close(tfe->fd_);
            }
        }
    }

}
