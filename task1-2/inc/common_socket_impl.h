#ifndef COMMON_SOCKET_IMPL_H_
#define COMMON_SOCKET_IMPL_H_

#include <assert.h>
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
#include <string>
#include <sstream>
#include <iostream>

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
#define SOCKET_STARTUP() static WSAStartupper wsa_startupper_;
#else
#define SOCKET_STARTUP()
#endif

std::string get_socket_error(int code);

std::string get_socket_error();

template<typename T>
void ensure_or_throw_impl(bool condition, const char *errname, const char *funname, const char *file, int line, const char *cond) {
  if (!condition) {
    std::stringstream msg;
    msg << errname << " in " << funname << "() at " << file << ":" << line << ": condition " << cond << " failed: " << get_socket_error();
    throw T(msg.str());
  }
}
#define ensure_or_throw(cond, error) ensure_or_throw_impl<error>(cond, #error, __FUNCTION__, __FILE__, __LINE__, #cond)

class NameResolver {
public:
  NameResolver(const char *host, int ai_family, int ai_socktype, int ai_protocol, int port = 0);
  ~NameResolver();

  const sockaddr* ai_addr() { return addrs->ai_addr; }
  int ai_addrlen() { return addrs->ai_addrlen; }

private:
  addrinfo *addrs;
};

inline std::ostream& operator<<(std::ostream &os, const sockaddr_in &addr) {
  return os << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
}

#endif
