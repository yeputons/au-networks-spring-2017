#ifndef CYCLIC_QUEUE_H
#define CYCLIC_QUEUE_H

#include <cstdint>
#include <type_traits>
#include <assert.h>

template<typename T, typename I, std::size_t max_size>
class cyclic_queue {
public:
  explicit cyclic_queue(I init_id = 0) : data_start_(0), data_end_(0), data_size_(0), id_start_(init_id), id_end_(init_id) {
    static_assert(std::is_unsigned<I>::value, "IDs inside cyclic_queue's should be unsigned to support integer overflows");
  }

  bool empty() const {
    return data_size_ == 0;
  }

  bool full() const {
    return data_size_ == max_size;
  }

  void reset_id(I new_id) {
    assert(empty());
    id_start_ = id_end_ = new_id;
  }

  bool contains(I id) const {
    if (data_size_ == 0) {
      return false;
    }
    if (id_start_ < id_end_) {
      return id_start_ <= id && id < id_end_;
    } else {
      return id_start_ <= id || id < id_end_;
    }
  }

  T& operator[](I id) {
    assert(contains(id));
    id -= id_start_;
    std::size_t pos = id + data_start_;
    if (pos >= max_size) {
      pos -= max_size;
    }
    assert(pos < max_size);
    return data_[pos];
  }

  T& front() {
    return data_[data_start_];
  }

  void pop_front() {
    assert(data_size_ >= 1);
    data_size_ -= 1;
    data_start_ += 1;
    id_start_ += 1;
    if (data_start_ >= max_size) {
      data_start_ -= max_size;
    }
  }

  void push_back(const T &value) {
    data_[data_end_] = value;
    assert(data_size_ + 1 <= max_size);
    data_size_ += 1;
    data_end_ += 1;
    id_end_ += 1;
    if (data_end_ >= max_size) {
      data_end_ -= max_size;
    }
  }

  I begin_id() const {
    return id_start_;
  }

  I end_id() const {
    return id_end_;
  }

private:
  T data_[max_size];
  std::size_t data_start_, data_end_, data_size_;
  I id_start_, id_end_;
};

#endif
