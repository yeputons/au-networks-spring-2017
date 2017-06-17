#include "utils/cyclic_queue.h"
#include <gtest/gtest.h>

const int64_t BILLION = 1000000000;

TEST(CyclicQueue, Init) {
  cyclic_queue<int64_t, uint8_t, 3> q;
  EXPECT_TRUE(q.empty());
  EXPECT_FALSE(q.full());
  EXPECT_EQ(0, q.begin_id());
  EXPECT_EQ(0, q.end_id());
  EXPECT_FALSE(q.contains(0));
  EXPECT_FALSE(q.contains(1));
}

TEST(CyclicQueue, ResetId) {
  cyclic_queue<int64_t, uint8_t, 3> q;

  q.reset_id(200);
  EXPECT_EQ(200, q.begin_id());
  EXPECT_EQ(200, q.end_id());
  EXPECT_FALSE(q.contains(0));
  EXPECT_FALSE(q.contains(200));
  EXPECT_FALSE(q.contains(201));
}

TEST(CyclicQueue, SeveralPushes) {
  cyclic_queue<int64_t, uint8_t, 3> q;
  q.reset_id(254);

  q.push_back(10 * BILLION);
  EXPECT_FALSE(q.empty());
  EXPECT_FALSE(q.full());
  EXPECT_EQ(254, q.begin_id());
  EXPECT_EQ(255, q.end_id());
  EXPECT_FALSE(q.contains(253));
  EXPECT_FALSE(q.contains(255));
  ASSERT_TRUE(q.contains(254));
  EXPECT_EQ(10 * BILLION, q[254]);

  q.push_back(20 * BILLION);
  EXPECT_FALSE(q.empty());
  EXPECT_FALSE(q.full());
  EXPECT_EQ(254, q.begin_id());
  EXPECT_EQ(0, q.end_id());
  EXPECT_FALSE(q.contains(253));
  EXPECT_FALSE(q.contains(0));
  ASSERT_TRUE(q.contains(254));
  ASSERT_TRUE(q.contains(255));
  EXPECT_EQ(10 * BILLION, q[254]);
  EXPECT_EQ(20 * BILLION, q[255]);

  q.push_back(30 * BILLION);
  EXPECT_FALSE(q.empty());
  EXPECT_TRUE(q.full());
  EXPECT_EQ(254, q.begin_id());
  EXPECT_EQ(1, q.end_id());
  EXPECT_FALSE(q.contains(253));
  EXPECT_FALSE(q.contains(1));
  ASSERT_TRUE(q.contains(254));
  ASSERT_TRUE(q.contains(255));
  ASSERT_TRUE(q.contains(0));
  EXPECT_EQ(10 * BILLION, q[254]);
  EXPECT_EQ(20 * BILLION, q[255]);
  EXPECT_EQ(30 * BILLION, q[0]);
}

TEST(CyclicQueue, SeveralPops) {
  cyclic_queue<int64_t, uint8_t, 3> q;
  q.reset_id(254);
  q.push_back(10 * BILLION);
  q.push_back(20 * BILLION);
  q.push_back(30 * BILLION);

  q.pop_front();
  EXPECT_FALSE(q.empty());
  EXPECT_FALSE(q.full());
  EXPECT_EQ(255, q.begin_id());
  EXPECT_EQ(1, q.end_id());
  EXPECT_FALSE(q.contains(254));
  EXPECT_FALSE(q.contains(1));
  ASSERT_TRUE(q.contains(255));
  ASSERT_TRUE(q.contains(0));
  EXPECT_EQ(20 * BILLION, q[255]);
  EXPECT_EQ(30 * BILLION, q[0]);

  q.pop_front();
  EXPECT_FALSE(q.empty());
  EXPECT_FALSE(q.full());
  EXPECT_EQ(0, q.begin_id());
  EXPECT_EQ(1, q.end_id());
  EXPECT_FALSE(q.contains(255));
  EXPECT_FALSE(q.contains(1));
  ASSERT_TRUE(q.contains(0));
  EXPECT_EQ(30 * BILLION, q[0]);

  q.pop_front();
  EXPECT_TRUE(q.empty());
  EXPECT_FALSE(q.full());
  EXPECT_EQ(1, q.begin_id());
  EXPECT_EQ(1, q.end_id());
  EXPECT_FALSE(q.contains(0));
  EXPECT_FALSE(q.contains(1));
}

TEST(CyclicQueue, Cycling) {
  cyclic_queue<int64_t, uint8_t, 3> q;
  q.reset_id(250);
  q.push_back(10 * BILLION);
  q.push_back(20 * BILLION);
  q.push_back(30 * BILLION);
  q.pop_front();
  q.push_back(40 * BILLION);
  q.pop_front();
  q.push_back(50 * BILLION);
  q.pop_front();
  q.pop_front();
  q.push_back(60 * BILLION);
  q.push_back(70 * BILLION);
  q.pop_front();
  q.push_back(80 * BILLION);
  q.pop_front();
  q.pop_front();

  EXPECT_FALSE(q.empty());
  EXPECT_FALSE(q.full());
  EXPECT_EQ(1, q.begin_id());
  EXPECT_EQ(2, q.end_id());
  ASSERT_TRUE(q.contains(1));
  EXPECT_EQ(80 * BILLION, q[1]);
}
