#ifndef PINK_THREAD_H_
#define PINK_THREAD_H_

#include <pthread.h>

class Thread
{

public:
    Thread();
    virtual ~Thread();
    void StartThread();


private:
    pthread_t thread_id_;

    static void *RunThread(void *arg);
    virtual void *ThreadMain() = 0; 
    
};

#endif
