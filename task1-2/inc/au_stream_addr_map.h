#ifndef AU_STREAM_ADDR_MAP_
#define AU_STREAM_ADDR_MAP_

#include <iostream>
namespace au_stream_socket {

struct sockaddr_in_cmp {
  bool operator()(const sockaddr_in &a, const sockaddr_in &b) const {
    assert(a.sin_family == AF_INET);
    assert(b.sin_family == AF_INET);
    if (a.sin_port != b.sin_port) {
      return a.sin_port < b.sin_port;
    }
    return a.sin_addr.s_addr < b.sin_addr.s_addr;
  }
};

template<typename T>
class addr_map {
private:
  typedef std::map<sockaddr_in, T, sockaddr_in_cmp> inner_map;

public:
  typedef typename inner_map::iterator iterator;

  iterator begin() { return data_.begin(); }
  iterator end() { return data_.end(); }
  bool contains(sockaddr_in addr) { return find(addr) != end(); }

  void insert(sockaddr_in addr, T value) {
    data_.insert(std::make_pair(addr, value));
  }

  void erase(iterator it) {
    data_.erase(it);
  }

  T& operator[](sockaddr_in addr) {
    auto it = find(addr);
    if (it == data_.end()) {
      return data_[addr];
    }
    return it->second;
  }

  iterator find(sockaddr_in addr) {
    assert(addr.sin_family == AF_INET);
    auto it = data_.lower_bound(addr);
    if (it != data_.end() && it->first.sin_port == addr.sin_port &&
        (it->first.sin_addr.s_addr == 0 || it->first.sin_addr.s_addr == addr.sin_addr.s_addr)) {
      return it;
    }
    if (addr.sin_addr.s_addr != 0) {
      sockaddr_in to_find = addr;
      to_find.sin_addr.s_addr = 0;
      it = data_.find(to_find);
      if (it != data_.end()) {
        return it;
      }
    }
    return data_.end();
  }

private:
  std::map<sockaddr_in, T, sockaddr_in_cmp> data_;
};

}  // namespace au_stream_socket

#endif
