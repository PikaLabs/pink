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
  std::cout << "time: " << time(NULL) << ", task : " << *((int *)arg) << std::endl;
}

int main() {
  int arg = 0;
  pink::Timer* timer = new pink::Timer(200, task, static_cast<void*>(&arg));
  std::cout << "#######Start timer" << std::endl;
  timer->Start();
  sleep(2);

  std::cout << "#######Stop timer" << std::endl;
  timer->Stop();
  std::cout << "#######sleep 5s ...: " << std::endl;
  sleep(5);

  std::cout << "#######Start timer" << std::endl;
  timer->Start();

  std::cout << "#######sleep 20 ms: " << std::endl;
  usleep(20 * 1000);
  std::cout << "#######Reset timer, remain: " << timer->RemainTime() << std::endl;
  timer->Reset();
  std::cout << "#######Has Reset timer, remain: " << timer->RemainTime() << std::endl;
  
  sleep(1);

  delete timer;
  
  return 0;
}
