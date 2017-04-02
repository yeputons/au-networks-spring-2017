#include <assert.h>
#include <memory.h>
#include <sstream>
#include "tcp_socket.h"
#ifdef _WIN32
#include <w32api.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#ifdef _WIN32
// INVALID_SOCKET is already defined
// SOCKET_ERROR is already defined
#else
static const int INVALID_SOCKET = -1;
static const int SOCKET_ERROR = -1;
#endif

tcp_connection_socket::tcp_connection_socket() : sock_(INVALID_SOCKET) {}

tcp_connection_socket::tcp_connection_socket(SOCKET sock) : sock_(sock) {}

tcp_connection_socket::tcp_connection_socket(tcp_connection_socket &&other) : sock_(other.sock_) {
  other.sock_ = static_cast<SOCKET>(0);
}

tcp_connection_socket& tcp_connection_socket::operator=(tcp_connection_socket other) {
  std::swap(sock_, other.sock_);
  return *this;
}

tcp_connection_socket::~tcp_connection_socket() {
  if (sock_ == INVALID_SOCKET) {
    return;
  }
  #ifdef _WIN32
  assert(closesocket(sock_) == 0);
  #else
  assert(close(sock_) == 0);
  #endif
}

void tcp_connection_socket::send(const void *buf, size_t size) {
  assert(sock_ != INVALID_SOCKET);
  for (size_t i = 0; i < size;) {
    int sent = ::send(sock_, static_cast<const char*>(buf) + i, size - i, 0);
    assert(sent != SOCKET_ERROR);
    i += sent;
  }
}

void tcp_connection_socket::recv(void *buf, size_t size) {
  assert(sock_ != INVALID_SOCKET);
  for (size_t i = 0; i < size;) {
    int recved = ::recv(sock_, static_cast<char*>(buf) + i, size - i, 0);
    assert(recved != SOCKET_ERROR);
    assert(recved != 0);  // Gracefully closed.
    i += recved;
  }
}

class NameResolver {
public:
  NameResolver(const char *host, tcp_port port) {
    addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::stringstream port_str;  // to_string in unavailable in MinGW.
    port_str << port;
    assert(getaddrinfo(host, port_str.str().c_str(), &hints, &addrs) == 0);
    assert(addrs != nullptr);
  }

  sockaddr* ai_addr() {
    return addrs->ai_addr;
  }

  int ai_addrlen() {
    return addrs->ai_addrlen;
  }

  ~NameResolver() {
    freeaddrinfo(addrs);
  }

private:
  addrinfo *addrs;
};

void tcp_client_socket::connect() {
  NameResolver resolver(host_.c_str(), port_);
  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(sock != INVALID_SOCKET);
  assert(::connect(sock, resolver.ai_addr(), resolver.ai_addrlen()) == 0);
  sock_ = tcp_connection_socket(sock);
}

tcp_server_socket::tcp_server_socket(hostname host, tcp_port port) {
  NameResolver resolver(host, port);
  sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(sock_ != INVALID_SOCKET);
  assert(::bind(sock_, resolver.ai_addr(), resolver.ai_addrlen()) == 0);
  assert(listen(sock_, SOMAXCONN) == 0);
}

tcp_server_socket::tcp_server_socket(tcp_server_socket &&other) : sock_(other.sock_) {
  other.sock_ = INVALID_SOCKET;
}
tcp_server_socket& tcp_server_socket::operator=(tcp_server_socket other) {
  std::swap(sock_, other.sock_);
  return *this;
}

stream_socket* tcp_server_socket::accept_one_client() {
  assert(sock_ != INVALID_SOCKET);
  sockaddr addr;
  socklen_t addrlen = sizeof(addr);
  SOCKET client = accept(sock_, &addr, &addrlen);
  assert(client != INVALID_SOCKET);
  return new tcp_connection_socket(client);
}

tcp_server_socket::~tcp_server_socket() {
  if (sock_ == INVALID_SOCKET) {
    return;
  }
  #ifdef _WIN32
  assert(closesocket(sock_) == 0);
  #else
  assert(close(sock_) == 0);
  #endif
}
