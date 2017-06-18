#include "utils/event.h"
#include "utils/retrier.h"
#include <chrono>
#include <mutex>
#include <gtest/gtest.h>
#include <assert.h>

typedef std::chrono::time_point<std::chrono::steady_clock> time_point;

TEST(Retrier, BasicMultipleTest) {
  std::mutex m;
  std::vector<std::pair<time_point, int>> stamps;
  
  retrier r;
  event ev1;
  int counter1 = 0;
  r.retry_after(std::chrono::milliseconds(8), [&m, &stamps, &ev1, &counter1]() {
    std::lock_guard<std::mutex> lock(m);
    stamps.push_back(std::make_pair(std::chrono::steady_clock::now(), 10 + counter1));
    counter1++;
    if (counter1 == 3) {
      ev1.notify();
      return true;
    } else {
      return false;
    }
  });

  event ev2;
  int counter2 = 0;
  r.retry_after(std::chrono::milliseconds(10), [&m, &stamps, &ev2, &counter2]() {
    std::lock_guard<std::mutex> lock(m);
    stamps.push_back(std::make_pair(std::chrono::steady_clock::now(), 20 + counter2));
    counter2++;
    if (counter2 == 2) {
      ev2.notify();
      return true;
    } else {
      return false;
    }
  });

  ev1.await();
  ev2.await();
  ASSERT_EQ(5, stamps.size());
  EXPECT_EQ(10, stamps[0].second); // 8ms
  EXPECT_EQ(20, stamps[1].second); // 10ms
  EXPECT_EQ(11, stamps[2].second); // 16ms
  EXPECT_EQ(21, stamps[3].second); // 20ms
  EXPECT_EQ(12, stamps[4].second); // 24ms
}

TEST(Retrier, Termination) {
  retrier r;
  r.retry_after(std::chrono::milliseconds(50), []() {
    assert(false);
    return false;
  });
}
