#ifndef SEMAPHORE_H
#define SEMAPHORE_H

class event {
public:
  event() : happened_(false) {
  }

  void await() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() {
      return happened_;
    });
  }

  void notify() {
    std::unique_lock<std::mutex> lock(mutex_);
    happened_ = true;
    cond_.notify_all();
  }

private:
  std::mutex mutex_;
  std::condition_variable cond_;
  bool happened_;
};

#endif  // SEMAPHORE_H
