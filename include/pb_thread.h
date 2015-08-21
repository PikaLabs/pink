#ifndef PINK_PB_THREAD_H_
#define PINK_PB_THREAD_H_

#include <string>
#include <functional>
#include <queue>
#include <map>

#include "pink_thread.h"
#include "pink_item.h"
#include "pink_epoll.h"
#include "pink_mutex.h"

class PbConn;

/*
 * template <typename T>
 */
class PbThread : public Thread
{
public:
    PbThread();
    virtual ~PbThread();

    /*
     * The PbItem queue is the fd queue, receive from dispatch thread
     */
    std::queue<PinkItem> conn_queue_;

    int notify_receive_fd() { return notify_receive_fd_; }
    int notify_send_fd() { return notify_send_fd_; }

    Mutex mutex_;
private:
    virtual void *ThreadMain(); 

/*
 *     typedef std::function<void(void)> handler;
 * 
 *     int Install(std::string &name, handler &h);
 * 
 *     std::map<std::string, handler> pbHandler_; 
 */

    /*
     * These two fd receive the notify from dispatch thread
     */
    int notify_receive_fd_;
    int notify_send_fd_;



    std::map<int, PbConn *> conns_;

    /*
     * The epoll handler
     */
    PinkEpoll *pinkEpoll_;
};

#endif
