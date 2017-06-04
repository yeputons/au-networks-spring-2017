#include "au_stream_socket_protocol.h"
#include "au_stream_socket_impl.h"
#include "common_socket_impl.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <string.h>

namespace au_stream_socket {

static const size_t MAX_SEGMENT_SIZE = 1000;

// Bytes  Description
// 0-1    source port (network order)
// 2-3    destination port (network order)
// 4-7    sequence number (network order)
// 8-11   ack sequence number (network order)
// 12     flags
// 13-15  reserved
// 16-19  checksum (all bytes of packet XORed together should yield zero)

int serialize(const au_packet &packet, char *buf, size_t len) {
  assert(20 + packet.data.size() <= len);

  memset(buf, 0, 20);
  *reinterpret_cast<uint16_t*>(buf + 0) = packet.source.sin_port;
  *reinterpret_cast<uint16_t*>(buf + 2) = packet.dest.sin_port;
  *reinterpret_cast<uint32_t*>(buf + 4) = htonl(packet.sn);
  *reinterpret_cast<uint32_t*>(buf + 8) = htonl(packet.ack_sn);
  *reinterpret_cast<Flags*>(buf + 12) = packet.flags;
  copy(packet.data.begin(), packet.data.end(), buf + 20);

  for (size_t checksum_pos = 0; checksum_pos < 4; checksum_pos++) {
    uint8_t expected = 0;
    for (size_t i = checksum_pos; i < len; i += 4) {
      expected ^= buf[i];
    }
    buf[16 + checksum_pos] = expected;
  }
  return 20 + packet.data.size();
}

au_packet deserialize(sockaddr_in source, sockaddr_in dest, char *buf, size_t len) {
  if (len < 20) {
    throw invalid_packet("Packet is too short");
  }
  for (size_t checksum_pos = 0; checksum_pos < 4; checksum_pos++) {
    uint8_t expected = 0;
    for (size_t i = checksum_pos; i < len; i += 4) {
      expected ^= buf[i];
    }
    if (expected) {
      throw invalid_packet("Invalid checksum");
    }
  }

  au_packet result;
  result.source = source;
  result.source.sin_port =       *reinterpret_cast<uint16_t*>(buf + 0);
  result.dest = dest;
  result.dest.sin_port   =       *reinterpret_cast<uint16_t*>(buf + 2);
  result.sn              = ntohl(*reinterpret_cast<uint32_t*>(buf + 4));
  result.ack_sn          = ntohl(*reinterpret_cast<uint32_t*>(buf + 8));
  result.flags           =       *reinterpret_cast<Flags*>(buf + 12);
  result.data.assign(buf + 20, buf + len);
  return result;
}

void messages_broker::process_packet(const au_packet &packet) {
  std::lock_guard<std::mutex> lock(mutex_);
  {
    auto it1 = connections_.find(packet.source);
    if (it1 != connections_.end()) {
      auto it2 = it1->second.find(packet.dest);
      if (it2 != it1->second.end()) {
        it2->second->process_packet(packet);
        return;
      }
    }
  }
  auto it = listeners_.find(packet.dest);
  assert(listeners_.begin() != listeners_.end());
  if (it != listeners_.end() && packet.flags == Flags::SYN) {
    auto &listener = it->second;
    auto conn = std::make_shared<connection_impl>(packet.dest, packet.source);
    conn->process_packet(packet);
    listener->add_client(conn);
    connections_[conn->get_remote()][conn->get_local()] = conn;
    return;
  }
  std::cerr << "Received unknown packet from " << packet.source << " to " << packet.dest
            << "; data_len=" << packet.data.size()
            << "; flags=" << static_cast<int>(packet.flags)
            << "\n";
}

connection_impl::connection_impl(sockaddr_in local, sockaddr_in remote)
  : sock_(), local_(local), remote_(remote), state_(CLOSED), recv_sn(0) {
  SOCKET_STARTUP();
  
  static std::default_random_engine engine;
  static std::mutex engine_mutex;
  {
    std::lock_guard<std::mutex> lock(engine_mutex);
    send_sn = engine();
  }

  sock_ = socket(AF_INET, SOCK_RAW, IPPROTO_AU);
  ensure_or_throw(sock_ != INVALID_SOCKET, socket_error);
  ensure_or_throw(bind(sock_, reinterpret_cast<sockaddr*>(&local_), sizeof(local_)) == 0, socket_error);
}

void connection_impl::send_packet(Flags flags, const std::vector<char> data) {
  au_packet ans;
  ans.source = local_;
  ans.dest = remote_;
  ans.flags = flags;
  ans.sn = send_sn;
  ans.ack_sn = recv_sn;
  ans.data = std::move(data);

  static thread_local char buf[20 + MAX_SEGMENT_SIZE];
  int len = serialize(ans, buf, sizeof buf);
  ensure_or_throw(sendto(sock_, buf, len, 0, reinterpret_cast<sockaddr*>(&remote_), sizeof(remote_)) == len, socket_io_error);
}

void connection_impl::start_connection() {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(state_ == CLOSED);

  send_packet(Flags::SYN, {});
  std::cerr << "Switch --> SYN_SENT\n";
  state_ = SYN_SENT;
}

void connection_impl::process_packet(const au_packet &packet) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == CLOSED && packet.flags == Flags::SYN) {
    recv_sn = packet.sn + 1;
    send_packet(Flags::SYN | Flags::ACK, {});
    state_ = SYN_RECV;
    std::cerr << "Switch CLOSED --> SYN_RECV\n";
  } else if (state_ == SYN_SENT && packet.flags == (Flags::SYN | Flags::ACK)) {
    if (send_sn + 1 != packet.ack_sn) {
      return;  // Fail
    }
    recv_sn = packet.sn + 1;
    send_packet(Flags::ACK, {});
    state_ = ESTABLISHED;
    std::cerr << "Switch SYN_SENT --> ESTABLISHED\n";
  } else if (state_ == SYN_RECV && packet.flags == Flags::ACK) {
    if (send_sn + 1 != packet.ack_sn || recv_sn != packet.sn + 1) {
      return;  // Fail
    }
    state_ = ESTABLISHED;
    std::cerr << "Switch SYN_RECV --> ESTABLISHED\n";
  } else {
    std::cerr << "Received some packet from " << packet.source << " to " << packet.dest
              << "; data_len=" << packet.data.size()
              << "; flags=" << static_cast<int>(packet.flags)
              << "\n";
    return;
  }
}

size_t connection_impl::send(const char *buf, size_t size) {
  // TODO
  (void)buf;
  (void)size;
  for(;;);
}

size_t connection_impl::recv(char *buf, size_t size) {
  // TODO
  (void)buf;
  (void)size;
  for(;;);
}

void connection_impl::shutdown() {
  // TODO
}

}  // namespace au_stream_socket
