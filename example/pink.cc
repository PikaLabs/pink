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
  BadaThread *badaThread[1];
  for (int i = 0; i < 1; i++) {
    badaThread[i] = new BadaThread();
  }
  Thread *t = new DispatchThread<BadaConn>(9211, 0, reinterpret_cast<WorkerThread<BadaConn> **>(badaThread));

  t->StartThread();


  sleep(1000);
  return 0;
}
