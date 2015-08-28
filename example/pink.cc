#include <stdio.h>
#include <unistd.h>

#include "pink_thread.h"
#include "pink_hb.h"
#include "pb_thread.h"
#include "dispatch_thread.h"
#include "pink.pb.h"
#include "bada_thread.h"

int main()
{
  BadaThread *badaThread[10];
  for (int i = 0; i < 10; i++) {
    badaThread[i] = new BadaThread();
  }
  Thread *t = new DispatchThread(9211, 1, reinterpret_cast<PbThread **>(badaThread));

  t->StartThread();


  sleep(1000);
  return 0;
}
