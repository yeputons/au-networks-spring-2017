#ifndef RETRIER_H
#define RETRIER_H

#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>

class retrier {
public:
  typedef std::chrono::milliseconds duration;
  typedef std::function<bool(void)> function;

  retrier();
  ~retrier();

  void retry_after(duration delay, function function);

private:
  typedef std::chrono::time_point<std::chrono::steady_clock> time_point;
  struct queue_item {
    time_point retry_at;
    duration delay;
    function func;

    bool operator>(const queue_item &other) const {
      return retry_at > other.retry_at;
    }
  };

  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::priority_queue<queue_item, std::vector<queue_item>, std::greater<queue_item>> queue_;
  bool terminating_;

  void run();
};

#endif  // RETRIER_H
