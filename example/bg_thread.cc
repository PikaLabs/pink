#include <string>
#include <iostream>
#include "bg_thread.h"
#include "unistd.h"

using namespace std;


void task(void *arg) {
  std::cout << "task : " << *((int *)arg) << std::endl;
  delete (int*)arg;
}


int main() {
  pink::BGThread t;
  t.StartThread();

  int i = 0;
  for (; i < 100; i++ ) {
    int *pi = new int(i);
    t.Schedule(task, (void*)pi);
  }
  sleep(1);

  return 1;
}
