#include "common_socket_impl.h"
#include "stream_socket.h"
#include <sstream>
#include <string>
#include <string.h>

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


NameResolver::NameResolver(const char *host, int ai_family, int ai_socktype, int ai_protocol, int port) {
  addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = ai_family;
  hints.ai_socktype = ai_socktype;
  hints.ai_protocol = ai_protocol;

  std::stringstream port_str;  // to_string in unavailable in MinGW.
  port_str << port;
  int result = getaddrinfo(host, port ? port_str.str().c_str() : nullptr, &hints, &addrs);
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

NameResolver::~NameResolver() {
  freeaddrinfo(addrs);
}
