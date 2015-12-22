#include <stdio.h>
#include <unistd.h>

#include "holy_thread.h"
#include "holy_test.h"
#include "pink.pb.h"

int main()
{
  Thread *t = new PinkThread(9211);

  t->StartThread();


  sleep(1000);
  return 0;
}
