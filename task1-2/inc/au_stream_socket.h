#ifndef AU_STREAM_SOCKET_H_
#define AU_STREAM_SOCKET_H_

#include <stdint.h>
#include <string>
#include <memory>
#include "stream_socket.h"

typedef uint16_t au_stream_port;

namespace au_stream_socket {
class connection_impl;
class listener_impl;
}

class au_stream_connection_socket : public stream_socket {
public:
  au_stream_connection_socket() {}
  au_stream_connection_socket(std::shared_ptr<au_stream_socket::connection_impl> impl) : impl_(impl) {}
  au_stream_connection_socket(au_stream_connection_socket &&other) = default;
  au_stream_connection_socket& operator=(au_stream_connection_socket &&other) = default;
  ~au_stream_connection_socket() override;

  explicit operator bool();
  void send(const void *buf, size_t size) override;
  void recv(void *buf, size_t size) override;

private:
  au_stream_connection_socket(const au_stream_connection_socket &) = delete;

  std::shared_ptr<au_stream_socket::connection_impl> impl_;
};

class au_stream_client_socket : public stream_client_socket {
public:
  au_stream_client_socket(hostname host, au_stream_port client_port, au_stream_port server_port);
  ~au_stream_client_socket() override {};

  void connect() override;
  void send(const void *buf, size_t size) override;
  void recv(void *buf, size_t size) override;

private:
  std::string host_;
  au_stream_port client_port_, server_port_;
  au_stream_connection_socket sock_;
};

class au_stream_server_socket : public stream_server_socket {
public:
  au_stream_server_socket(hostname host, au_stream_port port);
  au_stream_server_socket(au_stream_server_socket &&other) = default;
  au_stream_server_socket& operator=(au_stream_server_socket &&other) = default;
  ~au_stream_server_socket() override;

  stream_socket* accept_one_client() override;

private:
  au_stream_server_socket(const au_stream_server_socket &) = delete;

  std::shared_ptr<au_stream_socket::listener_impl> impl_;
};

typedef const char* hostname;

#endif  // AU_STREAM_SOCKET_H_
