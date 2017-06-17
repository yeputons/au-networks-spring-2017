#ifndef LOCKING_CHAR_QUEUE_H
#define LOCKING_CHAR_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <exception>
#include "utils/cyclic_queue.h"

struct locking_char_queue_shut_down : std::runtime_error {
  locking_char_queue_shut_down() : std::runtime_error("locking_char_queue was shut down") {}
};

template<std::size_t max_size>
class locking_char_queue {
public:
  locking_char_queue() : shutdown_(false) {}

  void shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    assert(!shutdown_);
    shutdown_ = true;
    send_cond_.notify_all();
    recv_cond_.notify_all();
  }

  void send(const char *buf, size_t size) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (size > 0) {
      send_cond_.wait(lock, [this]() {
        return shutdown_ || !queue.full();
      });
      if (shutdown_) {
        throw locking_char_queue_shut_down();
      }
      while (size > 0 && !queue.full()) {
        queue.push_back(*buf);
        buf++;
        size--;
      }
      recv_cond_.notify_one();
    }
    if (!queue.full()) {
      send_cond_.notify_one();
    }
  }

  void recv(char *buf, size_t size) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (size > 0) {
      recv_cond_.wait(lock, [this]() {
        return shutdown_ || !queue.empty();
      });
      size_t recved = try_recv_lock_held(buf, size);
      buf += recved;
      size -= recved;
    }
    if (!queue.empty()) {
      recv_cond_.notify_one();
    }
  }

  size_t try_recv(char *buf, size_t size) {
    std::unique_lock<std::mutex> lock(mutex_);
    return try_recv_lock_held(buf, size);
  }
  
private:
  size_t try_recv_lock_held(char *buf, size_t size) {
    if (shutdown_) {
      throw locking_char_queue_shut_down();
    }
    size_t recved = 0;
    while (size > 0 && !queue.empty()) {
      *buf = queue.front();
      queue.pop_front();
      buf++;
      size--;
      recved++;
    }
    send_cond_.notify_one();
    return recved;
  }

  std::mutex mutex_;
  std::condition_variable send_cond_, recv_cond_;
  bool shutdown_;
  cyclic_queue<char, std::size_t, max_size> queue;
};

#endif  // LOCKING_CHAR_QUEUE_H