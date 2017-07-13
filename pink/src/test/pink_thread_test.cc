#include "pink/include/pink_thread.h"

#include <stdlib.h>
#include <string>

#include "gmock/gmock.h"

using ::testing::AtLeast;
using ::testing::Invoke;

class MockThread : public pink::Thread {
 public:
  MOCK_METHOD0(ThreadMain, void*());

  void* thread_loop() {
    while (!should_stop()) {
      usleep(500);
    }
    return nullptr;
  }
};

TEST(PinkThreadTest, ThreadOps) {
  MockThread t;
  EXPECT_CALL(t, ThreadMain())
    .Times(AtLeast(1));

  ON_CALL(t, ThreadMain())
    .WillByDefault(Invoke(&t, &MockThread::thread_loop));

  EXPECT_EQ(0, t.StartThread());

  EXPECT_EQ(true, t.is_running());

  EXPECT_EQ(false, t.should_stop());

  EXPECT_EQ(0, t.StopThread());

  EXPECT_EQ(true, t.should_stop());

  EXPECT_EQ(false, t.is_running());
}
