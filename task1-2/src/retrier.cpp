#include "utils/retrier.h"

retrier::retrier() : thread_(&retrier::run, this), terminating_(false) {
}

retrier::~retrier() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    terminating_ = true;
    cond_.notify_one();
  }
  thread_.join();
}

void retrier::retry_after(duration delay, function function) {
  std::unique_lock<std::mutex> lock(mutex_);
  queue_.push({std::chrono::steady_clock::now() + delay, delay, std::move(function)});
  cond_.notify_one();
}

void retrier::run() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (true) {
    if (queue_.empty()) {
      cond_.wait(lock);
    } else {
      cond_.wait_until(lock, queue_.top().retry_at);
    }
    if (terminating_) {
      break;
    }
    while (!queue_.empty()) {
      if (std::chrono::steady_clock::now() < queue_.top().retry_at) {
        break;
      }
      duration delay = queue_.top().delay;
      function function = std::move(queue_.top().func);
      queue_.pop();
      if (!function()) {
        queue_.push({std::chrono::steady_clock::now() + delay, delay, std::move(function)});
      }
    }
  }
}
