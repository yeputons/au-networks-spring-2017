#include <sstream>
#include "tcp_socket.h"
#include "common_socket_impl.h"

tcp_connection_socket::tcp_connection_socket() : sock_(INVALID_SOCKET) {}

tcp_connection_socket::tcp_connection_socket(SOCKET sock) : sock_(sock) {}

tcp_connection_socket::tcp_connection_socket(tcp_connection_socket &&other) : sock_(other.sock_) {
  other.sock_ = INVALID_SOCKET;
}

tcp_connection_socket& tcp_connection_socket::operator=(tcp_connection_socket other) {
  std::swap(sock_, other.sock_);
  return *this;
}

tcp_connection_socket::~tcp_connection_socket() {
  if (sock_ == INVALID_SOCKET) {
    return;
  }
  assert(closesocket(sock_) == 0);
}

void tcp_connection_socket::send(const void *buf, size_t size) {
  ensure_or_throw(sock_ != INVALID_SOCKET, socket_uninitialized);
  for (size_t i = 0; i < size;) {
    int flags = 0;
    #ifdef __linux__
    flags |= MSG_NOSIGNAL;
    #endif
    int sent = ::send(sock_, static_cast<const char*>(buf) + i, size - i, flags);
    ensure_or_throw(sent != SOCKET_ERROR, socket_io_error);
    i += sent;
  }
}

void tcp_connection_socket::recv(void *buf, size_t size) {
  ensure_or_throw(sock_ != INVALID_SOCKET, socket_uninitialized);
  for (size_t i = 0; i < size;) {
    int recved = ::recv(sock_, static_cast<char*>(buf) + i, size - i, 0);
    ensure_or_throw(recved != SOCKET_ERROR, socket_io_error);
    if (recved == 0) {
      throw socket_eof_error("Socket was gracefully closed");
    }
    i += recved;
  }
}

void tcp_client_socket::connect() {
  SOCKET_STARTUP();

  NameResolver resolver(host_.c_str(), AF_INET, SOCK_STREAM, IPPROTO_TCP, port_);
  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  ensure_or_throw(sock != INVALID_SOCKET, socket_error);
  try {
    ensure_or_throw(::connect(sock, resolver.ai_addr(), resolver.ai_addrlen()) == 0, socket_error);
    sock_ = tcp_connection_socket(sock);
  } catch (...) {
    assert(closesocket(sock) == 0);
    throw;
  }
}

tcp_server_socket::tcp_server_socket(hostname host, tcp_port port) {
  SOCKET_STARTUP();

  NameResolver resolver(host, AF_INET, SOCK_STREAM, IPPROTO_TCP, port);
  sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  int reuse_addr = 1;
  ensure_or_throw(setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuse_addr), sizeof reuse_addr) == 0, socket_error);
  ensure_or_throw(sock_ != INVALID_SOCKET, socket_error);
  try {
    ensure_or_throw(::bind(sock_, resolver.ai_addr(), resolver.ai_addrlen()) == 0, socket_error);
    ensure_or_throw(listen(sock_, SOMAXCONN) == 0, socket_error);
  } catch (...) {
    assert(closesocket(sock_) == 0);
    throw;
  }
}

tcp_server_socket::tcp_server_socket(tcp_server_socket &&other) : sock_(other.sock_) {
  other.sock_ = INVALID_SOCKET;
}
tcp_server_socket& tcp_server_socket::operator=(tcp_server_socket other) {
  std::swap(sock_, other.sock_);
  return *this;
}

stream_socket* tcp_server_socket::accept_one_client() {
  ensure_or_throw(sock_ != INVALID_SOCKET, socket_uninitialized);
  sockaddr addr;
  socklen_t addrlen = sizeof(addr);
  SOCKET client = INVALID_SOCKET;
  for (int retry = 0; retry < 3; retry++) {
    client = accept(sock_, &addr, &addrlen);
    if (client != INVALID_SOCKET) {
      break;
    }
    #ifdef __linux__
    if (errno == EAGAIN ||
        errno == ENETDOWN ||
        errno == EPROTO ||
        errno == ENOPROTOOPT ||
        errno == EHOSTDOWN ||
        errno == ENONET ||
        errno == EHOSTUNREACH ||
        errno == EOPNOTSUPP ||
        errno == ENETUNREACH) {
      continue;  // Retry
    }
    #endif
    break;
  }
  ensure_or_throw(client != INVALID_SOCKET, socket_error);
  try {
    return new tcp_connection_socket(client);
  } catch (...) {
    assert(closesocket(client) == 0);
    throw;
  }
}

tcp_server_socket::~tcp_server_socket() {
  if (sock_ == INVALID_SOCKET) {
    return;
  }
  assert(closesocket(sock_) == 0);
}
