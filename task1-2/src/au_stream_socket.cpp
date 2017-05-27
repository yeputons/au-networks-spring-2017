#include <assert.h>
#include <thread>
#include <mutex>
#include <map>

#include "au_stream_socket.h"
#include "common_socket_impl.h"

class au_stream_socket_processor {
public:
  static au_stream_socket_processor& get() {
    static au_stream_socket_processor p;
    return p;
  }

  void run();

  std::shared_ptr<au_stream_socket_data> socket(au_stream_port port);
  void closesocket(std::shared_ptr<au_stream_socket_data> sock);

private:
  au_stream_socket_processor() : thread_(au_stream_socket_processor::run, this) {
  }
  ~au_stream_socket_processor() {
    assert(false);
  }

  std::thread thread_;
  std::mutex mutex_;
  std::map<au_stream_port, std::shared_ptr<au_stream_socket_data>> sockets_;
};

class au_stream_socket_data {
public:
  au_stream_socket_data(au_stream_port port) : port_(port) {}

  au_stream_port get_port() const {
    return port_;
  }

private:
  au_stream_port port_;
};

void au_stream_socket_processor::run() {
  // TODO: actually do something
}

std::shared_ptr<au_stream_socket_data> au_stream_socket_processor::socket(au_stream_port port) {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(port != 0);

  auto it = sockets_.lower_bound(port);
  if (it != sockets_.end() && it->first == port) {
    std::stringstream msg;
    msg << "Someone is already bound to port " << port;
    throw socket_error(msg.str().c_str());
  }

  auto impl = std::make_shared<au_stream_socket_data>(port);
  sockets_.emplace(port, impl);
  return impl;
}

void au_stream_socket_processor::closesocket(std::shared_ptr<au_stream_socket_data> sock) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = sockets_.find(sock->get_port());
  if (it == sockets_.end() || it->second != sock) {
    return;
  }
  // TODO: gracefully close the socket
  sockets_.erase(it);
}

au_stream_connection_socket::~au_stream_connection_socket() {
  auto impl = impl_.lock();
  if (impl) {
    au_stream_socket_processor::get().closesocket(impl);
  }
}

bool au_stream_connection_socket::bind(au_stream_port port) {
  auto impl = impl_.lock();
  if (impl) {
    return false;
  }
  impl_ = au_stream_socket_processor::get().socket(port);
  return true;
}

void au_stream_connection_socket::send(const void *buf, size_t size) {
  auto impl = impl_.lock();
  // TODO: actually send
  (void)buf;
  (void)size;
}
void au_stream_connection_socket::recv(void *buf, size_t size) {
  auto impl = impl_.lock();
  // TODO: actually recv
  (void)buf;
  (void)size;
}

au_stream_client_socket::au_stream_client_socket(hostname host, au_stream_port client_port, au_stream_port server_port)
  : host_(host)
  , client_port_(client_port)
  , server_port_(server_port) {}

void au_stream_client_socket::connect() {
  sock_.bind(client_port_);
  // TODO: actually connect
}

au_stream_server_socket::au_stream_server_socket(hostname host, au_stream_port port)
  : impl_(au_stream_socket_processor::get().socket(port)) {
  // TODO: start listening
  (void)host;
}

au_stream_server_socket::~au_stream_server_socket() {
  auto impl = impl_.lock();
  if (impl) {
    au_stream_socket_processor::get().closesocket(impl);
  }
}

stream_socket* au_stream_server_socket::accept_one_client() {
  // TODO: actually accept
  assert(false);
}
