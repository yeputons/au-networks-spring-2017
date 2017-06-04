#include "au_stream_socket_impl.h"
#include <iostream>

namespace au_stream_socket {

void messages_broker::start_listen(std::shared_ptr<listener_impl> sock) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (listeners_.contains(sock->get_addr())) {
    throw socket_error("Someone already listens on this local port");
  }
  listeners_.insert(sock->get_addr(), sock);
}

void messages_broker::stop_listen(sockaddr_in addr) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = listeners_.find(addr);
  assert(it != listeners_.end());
  listeners_.erase(it);
}

void messages_broker::add_connection(std::shared_ptr<connection_impl> sock) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto &conns = connections_[sock->get_remote()];
  if (conns.contains(sock->get_local())) {
    throw socket_error("There is already a similar connection");
  }
  conns.insert(sock->get_local(), sock);
}

//void messages_broker::remove_socket(std::shared_ptr<connection_impl>) {
//  // TODO
//}

void messages_broker::run() {
  SOCKET_STARTUP();
  SOCKET s = socket(AF_INET, SOCK_RAW, IPPROTO_AU);
  ensure_or_throw(s != INVALID_SOCKET, socket_error);

  sockaddr_in bind_addr;
  memset(&bind_addr, 0, sizeof bind_addr);
  bind_addr.sin_family = AF_INET;
  ensure_or_throw(bind(s, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)) == 0, socket_error);

  static thread_local char buffer[8192];
  for (;;) {
    sockaddr_in from;
    int fromlen = sizeof from;
    std::cerr << "recvfrom\n";
    int size = recvfrom(s, buffer, sizeof buffer, 0, reinterpret_cast<sockaddr*>(&from), &fromlen);
    ensure_or_throw(size > 0, socket_io_error);

    std::cerr << "size=" << size << "\n";
  }
}

listener_impl::listener_impl(sockaddr_in addr) : addr_(addr), shutdown_(false) {
}

listener_impl::~listener_impl() {
  assert(shutdown_);
}

sockaddr_in listener_impl::get_addr() const {
  return addr_;
}

void listener_impl::shutdown() {
  messages_broker::get().stop_listen(addr_);  

  std::lock_guard<std::mutex> lock(mutex_);
  shutdown_ = true;
  accept_wakeup_.notify_all();
}

std::shared_ptr<connection_impl> listener_impl::accept_one_client() {
  std::unique_lock<std::mutex> lock(mutex_);
  accept_wakeup_.wait(lock, [this]() {
    return shutdown_ || !clients_.empty();
  });
  if (!clients_.empty()) {
    auto result = clients_.front();
    clients_.pop();
    return result;
  }
  assert(shutdown_);
  throw socket_error("Listening socket was shut down");
}

connection_impl::connection_impl(sockaddr_in local, sockaddr_in remote)
  : local_(local), remote_(remote) {
}

sockaddr_in connection_impl::get_local() const {
  return local_;
}

sockaddr_in connection_impl::get_remote() const {
  return remote_;
}

void connection_impl::start_connection() {
  std::cerr << "start_connection\n";
  SOCKET_STARTUP();
  SOCKET s = socket(AF_INET, SOCK_RAW, IPPROTO_AU);
  ensure_or_throw(s != INVALID_SOCKET, socket_error);

  ensure_or_throw(bind(s, reinterpret_cast<sockaddr*>(&local_), sizeof(local_)) == 0, socket_error);

  char data[] = { 10, 20, 30, 40, 41 };
  ensure_or_throw(sendto(s, data, sizeof data, 0, reinterpret_cast<sockaddr*>(&remote_), sizeof(remote_)) == (sizeof data), socket_io_error);
  // TODO
}

size_t connection_impl::send(const char *buf, size_t size) {
  // TODO
  (void)buf;
  (void)size;
  for(;;);
}

size_t connection_impl::recv(char *buf, size_t size) {
  // TODO
  (void)buf;
  (void)size;
  for(;;);
}

void connection_impl::shutdown() {
  // TODO
}

}  // namespace au_stream_socket
