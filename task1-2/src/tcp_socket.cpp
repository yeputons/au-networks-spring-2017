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
#define closesocket close
#endif

#ifdef _WIN32
class WSAStartupper {
public:
  WSAStartupper() {
    WSADATA data;
    assert(WSAStartup(MAKEWORD(2, 2), &data) == 0);
  }
  // We do not call WSACleanup() because of troubles with order of global ctors/dtors.
private:
  WSAStartupper(WSAStartupper &&) = delete;
  WSAStartupper(const WSAStartupper&) = delete;
  WSAStartupper& operator=(WSAStartupper) = delete;
};
#endif

std::string get_socket_error(int code) {
  #ifdef _WIN32
  LPVOID msg_buf;
  assert(FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      code,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&msg_buf,
      0, NULL) != 0);
  std::string result((char*)msg_buf);
  LocalFree(msg_buf);
  return result;
  #else
  return strerror(code);
  #endif
}
std::string get_socket_error() {
  #ifdef _WIN32
  return get_socket_error(WSAGetLastError());
  #else
  return get_socket_error(errno);
  #endif
}

template<typename T>
void ensure_or_throw_impl(bool condition, const char *errname, const char *funname, const char *file, int line, const char *cond) {
  if (!condition) {
    std::stringstream msg;
    msg << errname << " in " << funname << "() at " << file << ":" << line << ": condition " << cond << " failed: " << get_socket_error();
    throw T(msg.str());
  }
}
#define ensure_or_throw(cond, error) ensure_or_throw_impl<error>(cond, #error, __FUNCTION__, __FILE__, __LINE__, #cond)

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
    int sent = ::send(sock_, static_cast<const char*>(buf) + i, size - i, 0);
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
    int result = getaddrinfo(host, port_str.str().c_str(), &hints, &addrs);
    if (result != 0) {
      std::stringstream msg;
      msg << "Unable to resolve host '" << host << "': " << get_socket_error(result);
      throw host_resolve_error(msg.str());
    }
    if (addrs == nullptr) {
      std::stringstream msg;
      msg << "Unable to resolve host '" << host << "': no matching host found";
      throw host_resolve_error(msg.str());
    }
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
  #ifdef _WIN32
  static WSAStartupper wsa_startupper_;
  #endif

  NameResolver resolver(host_.c_str(), port_);
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
  #ifdef _WIN32
  static WSAStartupper wsa_startupper_;
  #endif

  NameResolver resolver(host, port);
  sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
