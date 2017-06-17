#ifndef LOCKING_QUEUE_H
#define LOCKING_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <exception>
#include "utils/cyclic_queue.h"

struct locking_queue_shut_down : std::runtime_error {
  locking_queue_shut_down() : std::runtime_error("locking_queue was shut down") {}
};

template<typename T, typename I, std::size_t max_size>
class locking_queue {
public:
  locking_queue(std::mutex &mutex) : mutex_(mutex), shutdown_(false) {}

  void shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    assert(!shutdown_);
    shutdown_ = true;
    send_cond_.notify_all();
    recv_cond_.notify_all();
  }

  void send(const T *buf, size_t size) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (size > 0) {
      send_cond_.wait(lock, [this]() {
        return shutdown_ || !queue_.full();
      });
      size_t sent = try_send_lock_held(buf, size);
      buf += sent;
      size -= sent;
    }
    if (!queue_.full()) {
      send_cond_.notify_one();
    }
  }

  void recv(T *buf, size_t size) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (size > 0) {
      recv_cond_.wait(lock, [this]() {
        return shutdown_ || !queue_.empty();
      });
      size_t recved = try_recv_lock_held(buf, size);
      buf += recved;
      size -= recved;
    }
    if (!queue_.empty()) {
      recv_cond_.notify_one();
    }
  }

  size_t try_send(T *buf, size_t size) {
    std::unique_lock<std::mutex> lock(mutex_);
    return try_send_lock_held(buf, size);
  }

  size_t try_recv(T *buf, size_t size) {
    std::unique_lock<std::mutex> lock(mutex_);
    return try_recv_lock_held(buf, size);
  }

  cyclic_queue<T, I, max_size>& queue_lock_held() {
    return queue_;
  }
  
private:
  size_t try_send_lock_held(const T *buf, size_t size) {
    if (shutdown_) {
      throw locking_queue_shut_down();
    }
    size_t sent = 0;
    while (size > 0 && !queue_.full()) {
      queue_.push_back(*buf);
      buf++;
      size--;
      sent++;
    }
    recv_cond_.notify_one();
    return sent;
  }

  size_t try_recv_lock_held(T *buf, size_t size) {
    if (shutdown_) {
      throw locking_queue_shut_down();
    }
    size_t recved = 0;
    while (size > 0 && !queue_.empty()) {
      *buf = queue_.front();
      queue_.pop_front();
      buf++;
      size--;
      recved++;
    }
    send_cond_.notify_one();
    return recved;
  }

  std::mutex &mutex_;
  std::condition_variable send_cond_, recv_cond_;
  bool shutdown_;
  cyclic_queue<T, I, max_size> queue_;
};

#endif  // LOCKING_QUEUE_H