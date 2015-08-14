#include "pink_hb.h"

#include <unistd.h>
#include <stdio.h>

void *PinkHb::ThreadMain()
{
    while (1) {
        printf("pink thread\n");
        sleep(1);
    }

    return NULL;
}
