#include "common_socket_impl.h"
#include <sstream>
#include <string>

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
