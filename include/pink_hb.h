#ifndef PINK_HB_H_
#define PINK_HB_H_

#include "pink_thread.h"

class PinkHb : public Thread
{
    void *ThreadMain();
};

#endif
