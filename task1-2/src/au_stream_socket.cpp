#include <assert.h>
#include <thread>
#include <mutex>
#include <map>

#include "au_stream_socket.h"
#include "common_socket_impl.h"
#include "au_stream_socket_impl.h"

using au_stream_socket::messages_broker;

sockaddr_in resolve(hostname host, int port) {
  NameResolver resolver(host, AF_INET, SOCK_STREAM, au_stream_socket::IPPROTO_AU, port);
  assert(resolver.ai_addrlen() == sizeof(sockaddr_in));
  const sockaddr_in *addr = reinterpret_cast<const sockaddr_in*>(resolver.ai_addr());
  return *addr;
}

void au_stream_connection_socket::send(const void *orig_buf, size_t size) {
  if (!impl_) {
    throw socket_uninitialized("Socket is uninitialized");
  }
  const char *cbuf = static_cast<const char*>(orig_buf);
  while (size > 0) {
    size_t wrote = impl_->send(cbuf, size);
    if (!wrote) {
      throw std::logic_error("send() returned zero instead of blocking");
    }
    cbuf += wrote;
    size -= wrote;
  }
}

void au_stream_connection_socket::recv(void *orig_buf, size_t size) {
  if (!impl_) {
    throw socket_uninitialized("Socket is uninitialized");
  }
  char *cbuf = static_cast<char*>(orig_buf);
  while (size > 0) {
    size_t read = impl_->recv(cbuf, size);
    if (!read) {
      throw std::logic_error("recv() returned zero instead of blocking");
    }
    cbuf += read;
    size -= read;
  }
}

au_stream_connection_socket::operator bool() {
  return static_cast<bool>(impl_);
}

au_stream_connection_socket::~au_stream_connection_socket() {
  if (impl_) {
    impl_->shutdown();
  }
}

au_stream_client_socket::au_stream_client_socket(hostname host, au_stream_port client_port, au_stream_port server_port)
  : host_(host)
  , client_port_(client_port)
  , server_port_(server_port) {
}

void au_stream_client_socket::connect() {
  if (sock_) {
    // Ignore, already connected
    return;
  }

  sockaddr_in local_addr;
  memset(&local_addr, 0, sizeof local_addr);
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(client_port_);

  auto impl = std::make_shared<au_stream_socket::connection_impl>(local_addr, resolve(host_.c_str(), server_port_));
  messages_broker::get().add_connection(impl);
  impl->start_connection();
  sock_ = au_stream_connection_socket(impl);
}

void au_stream_client_socket::send(const void *buf, size_t size) {
  if (!sock_) {
    throw socket_uninitialized("Socket is not connected");
  }
  sock_.send(buf, size);
}

void au_stream_client_socket::recv(void *buf, size_t size) {
  if (!sock_) {
    throw socket_uninitialized("Socket is not connected");
  }
  sock_.recv(buf, size);
}

au_stream_server_socket::au_stream_server_socket(hostname host, au_stream_port port)
    : impl_(std::make_shared<au_stream_socket::listener_impl>(resolve(host, port))) {
  messages_broker::get().start_listen(impl_);
}

au_stream_server_socket::~au_stream_server_socket() {
  impl_->shutdown();
}

stream_socket* au_stream_server_socket::accept_one_client() {
  return new au_stream_connection_socket(impl_->accept_one_client());
}
