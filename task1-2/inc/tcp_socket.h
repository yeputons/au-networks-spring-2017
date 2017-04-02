#ifndef TCP_SOCKET_H_
#define TCP_SOCKET_H_

#include <stdint.h>
#include <string>
#include "stream_socket.h"

#ifdef _WIN32
#include <winsock2.h>
#else
typedef int SOCKET;
#endif

typedef uint16_t tcp_port;

class tcp_connection_socket : public stream_socket {
public:
  tcp_connection_socket();
  tcp_connection_socket(SOCKET sock);  // Takes ownership of sock.
  tcp_connection_socket(tcp_connection_socket &&other);
  tcp_connection_socket& operator=(tcp_connection_socket other);
  ~tcp_connection_socket() override;

  void send(const void *buf, size_t size) override;
  void recv(void *buf, size_t size) override;

private:
  tcp_connection_socket(const tcp_connection_socket &) = delete;

  SOCKET sock_;
};

class tcp_client_socket : public stream_client_socket {
public:
  tcp_client_socket(hostname host, tcp_port port) : host_(host), port_(port) {}
  ~tcp_client_socket() override {};

  void connect() override;
  void send(const void *buf, size_t size) override { sock_.send(buf, size); }
  void recv(void *buf, size_t size) override { sock_.recv(buf, size); }

private:
  std::string host_;
  tcp_port port_;
  tcp_connection_socket sock_;
};

class tcp_server_socket : public stream_server_socket {
public:
  tcp_server_socket(hostname host, tcp_port port);
  tcp_server_socket(tcp_server_socket &&other);
  tcp_server_socket& operator=(tcp_server_socket other);
  ~tcp_server_socket() override;

  stream_socket* accept_one_client() override;

private:
  tcp_server_socket(const tcp_server_socket &) = delete;

  SOCKET sock_;
};

typedef const char* hostname;

#endif  // TCP_SOCKET_H_
