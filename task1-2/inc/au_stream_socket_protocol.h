#ifndef AU_STREAM_SOCKET_PROTOCOL_H_
#define AU_STREAM_SOCKET_PROTOCOL_H_

#include <vector>
#include <stdexcept>
#include "common_socket_impl.h"

namespace au_stream_socket {

enum class Flags : uint8_t {
  NONE = 0,
  SYN = 1,
  ACK = 2
};

inline Flags operator|(Flags a, Flags b) {
  return static_cast<Flags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline Flags operator&(Flags a, Flags b) {
  return static_cast<Flags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

struct au_packet {
  sockaddr_in source, dest;
  uint32_t sn, ack_sn;
  Flags flags;
  std::vector<char> data;
};

class invalid_packet : public std::runtime_error {
public:
  invalid_packet(const char *msg) : std::runtime_error(msg) {}
};

int serialize(const au_packet &packet, char *buf, size_t len);

au_packet deserialize(sockaddr_in source, sockaddr_in dest, char *buf, size_t len);

}  // namespace au_stream_socket

#endif  // AU_STREAM_SOCKET_PROTOCOL_H_