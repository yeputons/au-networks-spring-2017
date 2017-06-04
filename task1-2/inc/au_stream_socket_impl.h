#ifndef AU_STREAM_SOCKET_IMPL_H_
#define AU_STREAM_SOCKET_IMPL_H_

#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <map>
#include "au_stream_socket.h"
#include "common_socket_impl.h"
#include "au_stream_addr_map.h"

namespace au_stream_socket {

const int IPPROTO_AU = 151;

class messages_broker {
public:
  static messages_broker& get() {
    static messages_broker p;
    return p;
  }

  void run();

  void start_listen(std::shared_ptr<listener_impl> sock);
  void stop_listen(sockaddr_in addr);

  void add_connection(std::shared_ptr<connection_impl> sock);
  void remove_connection(sockaddr_in local, sockaddr_in remote);

private:
  messages_broker() : thread_(&messages_broker::run, this) {
    thread_.detach();
  }

  std::thread thread_;
  std::mutex mutex_;
  addr_map<std::shared_ptr<listener_impl>> listeners_;
  addr_map<addr_map<std::shared_ptr<connection_impl>>> connections_;
};

class listener_impl {
public:
  listener_impl(sockaddr_in addr);
  ~listener_impl();
  sockaddr_in get_addr() const;
  void shutdown();

  void process_packet(const char *data, size_t len);

  std::shared_ptr<connection_impl> accept_one_client();

private:
  sockaddr_in addr_;
  bool shutdown_;

  std::mutex mutex_;
  std::condition_variable accept_wakeup_;
  std::queue<std::shared_ptr<connection_impl>> clients_;
};

class connection_impl {
public:
  connection_impl(sockaddr_in local, sockaddr_in remote);
  sockaddr_in get_local() const;
  sockaddr_in get_remote() const;

  void process_packet(const char *data, size_t len);

  void start_connection();
  size_t send(const char *buf, size_t size);
  size_t recv(char *buf, size_t size);
  void shutdown();

private:
  sockaddr_in local_, remote_;
};

}  // namespace au_stream_socket

#endif  // AU_STREAM_SOCKET_IMPL_H_