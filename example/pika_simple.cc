#include <stdio.h>
#include <unistd.h>

#include "pika_simple_thread.h"

using namespace pink;
int main()
{
  Thread *t = new PikaSimpleThread(6);

  t->StartThread();


  sleep(2);
  return 0;
}
