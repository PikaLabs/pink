#include "pb_thread.h"

virtual void *ThreadMain()
{
    thread_id_ = pthread_self();
    int nfds;
    TickFiredEvent *tfe = NULL;
    char bb[1];
    TickItem ti;
    TickConn *inConn;
    for (;;) {
        nfds = tickEpoll_->TickPoll();
        log_info("nfds %d", nfds);
        for (int i = 0; i < nfds; i++) {
            tfe = (tickEpoll_->firedevent()) + i;
            log_info("tfe->fd_ %d tfe->mask_ %d", tfe->fd_, tfe->mask_);
            if (tfe->fd_ == notify_receive_fd_ && (tfe->mask_ & EPOLLIN)) {
                read(notify_receive_fd_, bb, 1);
                {
                MutexLock l(&mutex_);
                ti = conn_queue_.front();
                conn_queue_.pop();
                }
                TickConn *tc = new TickConn(ti.fd());
                tc->SetNonblock();
                conns_[ti.fd()] = tc;

                tickEpoll_->TickAddEvent(ti.fd(), EPOLLIN);
                log_info("receive one fd %d", ti.fd());
                /*
                 * tc->set_thread(this);
                 */
            }
            int shouldClose = 0;
            if (tfe->mask_ & EPOLLIN) {
                inConn = conns_[tfe->fd_];
                // log_info("come if readable %d", (inConn == NULL));
                if (inConn == NULL) {
                    continue;
                }
                if (inConn->TickGetRequest() == 0) {
                    tickEpoll_->TickModEvent(tfe->fd_, 0, EPOLLOUT);
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
                if (inConn->TickSendReply() == 0) {
                    log_info("SendReply ok");
                    tickEpoll_->TickModEvent(tfe->fd_, 0, EPOLLIN);
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
