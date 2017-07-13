#include "pink/include/pink_thread.h"

#include <string>

#include "gmock/gmock.h"

using namespace pink;
using ::testing::AtLeast;
using ::testing::Return;

class Test {
  virtual int func(int a) = 0;
};

class MockTest : public Test {
 public:
  MOCK_METHOD1(func, int(int));
  // MOCK_METHOD0(StopThread, int());
  // MOCK_METHOD0(JoinThread, int());
  // MOCK_METHOD0(should_stop, bool());
  // MOCK_METHOD0(set_should_stop, void());
  // MOCK_METHOD0(is_running, bool());
  // MOCK_CONST_METHOD0(thread_id, int());
  // MOCK_CONST_METHOD0(thread_name, std::string());
  // MOCK_METHOD1(set_thread_name, void(const std::string& name));
  // MOCK_METHOD0(ThreadMain, void*());
};

TEST(PinkThread, test1) {
  // MockThread mock_t;

  // ON_CALL(mock_t, StartThread())
  //   .WillByDefault(Return(0));

  // ON_CALL(mock_t, StopThread())
  //   .WillByDefault(Return(0));

  // EXPECT_EQ(0, mock_t.StartThread());
  // mock_t.StartThread();

  MockTest t;
  EXPECT_CALL(t, func(12))
    .Times(AtLeast(1));

  ON_CALL(t, func(12))
    .WillByDefault(Return(1));

  EXPECT_EQ(1, t.func(12));
}
