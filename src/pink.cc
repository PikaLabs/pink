#include <stdio.h>
#include <unistd.h>

#include "pink_thread.h"
#include "pink_hb.h"
#include "pb_thread.h"
#include "dispatch_thread.h"

int main()
{
    
    Thread *t = new DispatchThread(9211, 1);

    t->StartThread();

    while(1);
    return 0;
}
