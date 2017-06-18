#include "utils/locking_queue.h"
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <atomic>
#include <assert.h>
#include "utils/event.h"

TEST(LockingCharQueue, SingleThreadSmoke) {
  std::mutex m;
  locking_queue<char, std::size_t, 6> q(m);
  char buffer[] = { 1, 2, 3, 4, 5 };
  q.send(buffer, 3);
  q.send(buffer + 2, 3);

  char recved[5] = {};
  q.recv(recved, 5);
  EXPECT_EQ(1, recved[0]);
  EXPECT_EQ(2, recved[1]);
  EXPECT_EQ(3, recved[2]);
  EXPECT_EQ(3, recved[3]);
  EXPECT_EQ(4, recved[4]);

  q.recv(recved, 1);
  EXPECT_EQ(5, recved[0]);

  q.shutdown();
}

TEST(LockingCharQueue, TrySend) {
  std::mutex m;
  locking_queue<char, std::size_t, 4> q(m);
  char buffer[] = { 1, 2, 3, 4, 5 };
  ASSERT_EQ(3, q.try_send(buffer, 3));
  ASSERT_EQ(1, q.try_send(buffer + 2, 3));

  char recved[4] = {};
  q.recv(recved, 4);
  EXPECT_EQ(1, recved[0]);
  EXPECT_EQ(2, recved[1]);
  EXPECT_EQ(3, recved[2]);
  EXPECT_EQ(3, recved[3]);

  ASSERT_EQ(4, q.try_send(buffer, 5));
  q.recv(recved, 4);
  EXPECT_EQ(1, recved[0]);
  EXPECT_EQ(2, recved[1]);
  EXPECT_EQ(3, recved[2]);
  EXPECT_EQ(4, recved[3]);

  q.shutdown();
}

TEST(LockingCharQueue, TryRecv) {
  std::mutex m;
  locking_queue<char, std::size_t, 6> q(m);
  char buffer[] = { 1, 2, 3, 4, 5 };
  q.send(buffer, 3);
  q.send(buffer + 2, 3);

  char recved[5] = {};
  ASSERT_EQ(4, q.try_recv(recved, 4));
  EXPECT_EQ(1, recved[0]);
  EXPECT_EQ(2, recved[1]);
  EXPECT_EQ(3, recved[2]);
  EXPECT_EQ(3, recved[3]);

  ASSERT_EQ(2, q.try_recv(recved, 4));
  EXPECT_EQ(4, recved[0]);
  EXPECT_EQ(5, recved[1]);

  ASSERT_EQ(0, q.try_recv(recved, 4));

  q.send(buffer, 1);

  ASSERT_EQ(1, q.try_recv(recved, 4));
  EXPECT_EQ(1, recved[0]);

  q.shutdown();
}

TEST(LockingCharQueue, ProducerConsumer) {
  const int TRANSMISSION_SIZE = 100000;
  const int MIN_BLOCK_SIZE = 1;
  const int MAX_BLOCK_SIZE = 20;

  std::mutex m;
  locking_queue<char, std::size_t, 16> q(m);
  std::thread producer([&q]() {
    std::default_random_engine gen;
    std::uniform_int_distribution<int> distrib(MIN_BLOCK_SIZE, MAX_BLOCK_SIZE);
    for (int i = 0; i < TRANSMISSION_SIZE;) {
      char block[MAX_BLOCK_SIZE];
      int block_size = std::min(TRANSMISSION_SIZE - i, distrib(gen));
      for (int in_block = 0; in_block < block_size; in_block++) {
        block[in_block] = static_cast<unsigned char>(i);
        i++;
      }
      q.send(block, block_size);
    }
  });

  std::vector<char> received;
  bool consumer_stopped;
  std::thread consumer([&q, &received, &consumer_stopped]() {
    std::default_random_engine gen;
    std::uniform_int_distribution<int> distrib(MIN_BLOCK_SIZE, MAX_BLOCK_SIZE);
    for (int i = 0; i < TRANSMISSION_SIZE;) {
      char block[MAX_BLOCK_SIZE];
      int block_size = std::min(TRANSMISSION_SIZE - i, distrib(gen));
      q.recv(block, block_size);
      received.insert(received.end(), block, block + block_size);
      i += block_size;
    }
  });

  producer.join();
  consumer.join();
  q.shutdown();

  ASSERT_EQ(TRANSMISSION_SIZE, received.size());
  for (int i = 0; i < TRANSMISSION_SIZE; i++) {
    ASSERT_EQ(static_cast<unsigned char>(i), static_cast<unsigned char>(received[i]));
  }
}

TEST(LockingCharQueue, ShutdownAbortsSend) {
  std::mutex m;
  locking_queue<char, std::size_t, 10> q(m);
  event ev;
  bool aborted = false;
  std::thread producer([&q, &ev, &aborted]() {
    char block[10] = {};
    q.send(block, 5);
    ev.notify();
    try {
      q.send(block, 6);
    } catch (const locking_queue_shut_down&) {
      aborted = true;
    }
  });
  ev.await();

  EXPECT_FALSE(aborted);
  q.shutdown();
  producer.join();
  EXPECT_TRUE(aborted);
}

TEST(LockingCharQueue, ShutdownAbortsRecv) {
  std::mutex m;
  locking_queue<char, std::size_t, 10> q(m);
  event ev;
  bool aborted = false;
  std::thread consumer([&q, &ev, &aborted]() {
    char block[10] = {};
    q.recv(block, 5);
    ev.notify();
    try {
      q.recv(block, 6);
    } catch (const locking_queue_shut_down&) {
      aborted = true;
    }
  });
  {
    char block[10] = {};
    q.send(block, sizeof block);
  }
  ev.await();

  EXPECT_FALSE(aborted);
  q.shutdown();
  consumer.join();
  EXPECT_TRUE(aborted);
}
