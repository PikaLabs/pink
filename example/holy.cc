#include <stdio.h>
#include <unistd.h>

#include "holy_thread.h"
#include "holy_test.h"
#include "pink.pb.h"

using namespace pink;

int main()
{
  Thread *t = new PinkThread(9211, 3000);
  t->set_thread_name("new_thread_name");

  t->StartThread();


  sleep(1000);
  return 0;
}
