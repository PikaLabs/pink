// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "unistd.h"
#include <string>
#include <iostream>
#include "pink/include/bg_thread.h"

using namespace std;


void task(void *arg) {
  std::cout << "task : " << *((int *)arg) << std::endl;
  delete (int*)arg;
}

struct TimerItem {
  uint64_t exec_time;
  void (*function)(void *);
  void* arg;
  TimerItem(uint64_t _exec_time, void (*_function)(void*), void* _arg) :
    exec_time(_exec_time),
    function(_function),
    arg(_arg) {}
  bool operator < (const TimerItem& item) const {
    return exec_time > item.exec_time;
  }
};

int main() {
  pink::BGThread *t = new pink::BGThread();
  t->StartThread();

  for (int i = 0; i < 100; i++) {
    int *pi = new int(i);
    t->Schedule(task, (void*)pi);
  }
  sleep(1);
  
  std::priority_queue<TimerItem> pq;
  pq.push(TimerItem(1, task, NULL));
  pq.push(TimerItem(5, task, NULL));
  pq.push(TimerItem(3, task, NULL));
  pq.push(TimerItem(2, task, NULL));
  pq.push(TimerItem(4, task, NULL));

  while (!pq.empty()) {
    printf("%ld\n", pq.top().exec_time);
    pq.pop();
  }

  for (int i = 0; i < 100; i++) {
    int *pi = new int(i);
    t->DelaySchedule(i * 1000, task, (void*)pi);
  }
  sleep(20);

  return 0;
}
