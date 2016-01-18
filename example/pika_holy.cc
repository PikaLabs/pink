#include <stdio.h>
#include <unistd.h>

#include "pika_holy_thread.h"

int main()
{
  Thread *t = new PikaHolyThread(9211, 1000);

  t->StartThread();


  sleep(1000);
  return 0;
}
