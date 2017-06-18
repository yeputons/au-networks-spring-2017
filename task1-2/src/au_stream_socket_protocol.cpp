#include "au_stream_socket_protocol.h"
#include "au_stream_socket_impl.h"
#include "common_socket_impl.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <random>
#include <string.h>

namespace au_stream_socket {

static const size_t MAX_SEGMENT_SIZE = 1000;
static const size_t AU_PACKET_HEADER_SIZE = 20;
static const retrier::duration SEND_ACK_TIMEOUT = std::chrono::milliseconds(100);

// Bytes  Description
// 0-1    source port (network order)
// 2-3    destination port (network order)
// 4-7    sequence number (network order)
// 8-11   ack sequence number (network order)
// 12     flags
// 13-15  reserved
// 16-19  checksum (all bytes of packet XORed together should yield zero)

int serialize(const au_packet &packet, char *buf, size_t len) {
  assert(AU_PACKET_HEADER_SIZE + packet.data.size() <= len);
  len = AU_PACKET_HEADER_SIZE + packet.data.size();

  memset(buf, 0, AU_PACKET_HEADER_SIZE);
  *reinterpret_cast<uint16_t*>(buf + 0) = packet.source.sin_port;
  *reinterpret_cast<uint16_t*>(buf + 2) = packet.dest.sin_port;
  *reinterpret_cast<uint32_t*>(buf + 4) = htonl(packet.sn);
  *reinterpret_cast<uint32_t*>(buf + 8) = htonl(packet.ack_sn);
  *reinterpret_cast<Flags*>(buf + 12) = packet.flags;
  copy(packet.data.begin(), packet.data.end(), buf + AU_PACKET_HEADER_SIZE);

  for (size_t checksum_pos = 0; checksum_pos < 4; checksum_pos++) {
    uint8_t expected = 0;
    for (size_t i = checksum_pos; i < len; i += 4) {
      expected ^= buf[i];
    }
    buf[16 + checksum_pos] = expected;
  }
  return len;
}

au_packet deserialize(sockaddr_in source, sockaddr_in dest, char *buf, size_t len) {
  if (len < AU_PACKET_HEADER_SIZE) {
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
  result.data.assign(buf + AU_PACKET_HEADER_SIZE, buf + len);
  return result;
}

void messages_broker::process_packet(au_packet packet) {
  std::lock_guard<std::mutex> lock(mutex_);
  {
    auto it1 = connections_.find(packet.source);
    if (it1 != connections_.end()) {
      auto it2 = it1->second.find(packet.dest);
      if (it2 != it1->second.end()) {
        it2->second->process_packet(std::move(packet));
        return;
      }
    }
  }
  auto it = listeners_.find(packet.dest);
  assert(listeners_.begin() != listeners_.end());
  if (it != listeners_.end() && packet.flags == Flags::SYN) {
    auto &listener = it->second;
    auto conn = std::make_shared<connection_impl>(packet.dest, packet.source);
    conn->process_packet(std::move(packet));
    listener->add_client(conn);
    add_connection_lock_held(conn);
    return;
  }
  std::cerr << "Received unknown packet from " << packet.source << " to " << packet.dest
            << "; data_len=" << packet.data.size()
            << "; flags=" << static_cast<int>(packet.flags)
            << "\n";
}

connection_impl::connection_impl(sockaddr_in local, sockaddr_in remote)
  : sock_(), local_(local), remote_(remote), state_(CLOSED), ack_sn(0), send_window(mutex_), recv_queue(mutex_) {
  SOCKET_STARTUP();
  send_window.queue_lock_held().reset_id(10);  // Arbitrary const greater than zero (so we can call send_window.begin_id() - 1)
  sock_ = socket(AF_INET, SOCK_RAW, IPPROTO_AU);
  ensure_or_throw(sock_ != INVALID_SOCKET, socket_error);
  ensure_or_throw(bind(sock_, reinterpret_cast<sockaddr*>(&local_), sizeof(local_)) == 0, socket_error);
}

void connection_impl::send_packet(Flags flags, uint32_t sn, std::vector<char> data) {
  au_packet ans;
  ans.source = local_;
  ans.dest = remote_;
  ans.flags = flags;
  ans.sn = sn;
  ans.ack_sn = ack_sn;
  ans.data = std::move(data);

  static thread_local char buf[AU_PACKET_HEADER_SIZE + MAX_SEGMENT_SIZE];
  int len = serialize(ans, buf, sizeof buf);
  ensure_or_throw(sendto(sock_, buf, len, 0, reinterpret_cast<sockaddr*>(&remote_), sizeof(remote_)) == len, socket_io_error);
}

void connection_impl::start_connection() {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(state_ == CLOSED);

  send_packet(Flags::SYN, send_window.queue_lock_held().begin_id() - 1, {});
  // TODO: add retry after timeout
  std::cout << "Switch --> SYN_SENT\n";
  state_ = SYN_SENT;
}

void connection_impl::process_packet(au_packet packet) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto &send_window_queue = send_window.queue_lock_held();
  if (state_ == CLOSED && packet.flags == Flags::SYN) {
    if (!packet.data.empty()) {
      return;  // Fail
    }
    ack_sn = packet.sn + 1;
    send_packet(Flags::SYN | Flags::ACK, send_window_queue.begin_id() - 1, {});
    // TODO: add retry after timeout
    state_ = SYN_RECV;
    std::cout << "Switch CLOSED --> SYN_RECV\n";
  } else if (state_ == SYN_SENT && packet.flags == (Flags::SYN | Flags::ACK)) {
    if (!packet.data.empty()) {
      return;  // Fail
    }
    if (send_window_queue.begin_id() != packet.ack_sn) {
      return;  // Fail
    }
    ack_sn = packet.sn + 1;
    send_packet(Flags::ACK, send_window_queue.begin_id(), {});
    state_ = ESTABLISHED;
    std::cout << "Switch SYN_SENT --> ESTABLISHED\n";
  } else if (state_ == SYN_RECV && packet.flags == Flags::ACK) {
    if (!packet.data.empty()) {
      return;  // Fail
    }
    if (send_window_queue.begin_id() != packet.ack_sn || ack_sn != packet.sn) {
      return;  // Fail
    }
    state_ = ESTABLISHED;
    std::cout << "Switch SYN_RECV --> ESTABLISHED\n";
  } else if (packet.flags == Flags::FIN) {
    send_packet(Flags::FIN | Flags::ACK, send_window_queue.begin_id(), {});
    // TODO: add retry after timeout
    state_ = FIN_RECV;
    std::cout << "Switching --> FIN_RECV\n";
  } else if (state_ == FIN_SENT && packet.flags == (Flags::FIN | Flags::ACK)) {
    send_packet(Flags::ACK, send_window_queue.begin_id(), {});
    // TODO: add retry after timeout
    state_ = TERMINATED;
    std::cout << "Switching FIN_SENT --> TERMINATED\n";
    messages_broker::get().remove_connection_from_process_packet(get_local(), get_remote());
    send_window.shutdown_lock_held();
    recv_queue.shutdown_lock_held();
  } else if (state_ == FIN_RECV && packet.flags == Flags::ACK) {
    state_ = TERMINATED;
    std::cout << "Switching FIN_RECV --> TERMINATED\n";
    messages_broker::get().remove_connection_from_process_packet(get_local(), get_remote());
    send_window.shutdown_lock_held();
    recv_queue.shutdown_lock_held();
  } else if (state_ == ESTABLISHED) {
    bool send_window_changed = false;

    if ((packet.flags & Flags::ACK) == Flags::ACK) {
      #ifdef AU_DEBUG
      std::cout << "Received confirmation till " << packet.ack_sn << "\n";
      #endif

      while (send_window_queue.begin_id() != packet.ack_sn && (send_window_queue.contains(packet.ack_sn) || send_window_queue.end_id() == packet.ack_sn)) {
        std::size_t to_ack = packet.ack_sn - send_window_queue.begin_id();
        assert(to_ack);

        static thread_local char buf[MAX_SEGMENT_SIZE];
        try {
          std::size_t acked = send_window.try_recv_lock_held(buf, std::min(sizeof(buf), to_ack));
          assert(acked == to_ack);
          #ifdef AU_DEBUG
          std::cout << "Dropped " << acked << " bytes out of send buffer, remaining: " << send_window_queue.begin_id() << " to " << send_window_queue.end_id() << "\n";
          #endif
          send_window_changed = true;
        } catch (const locking_queue_shut_down&) {
          break;
        }
      }
    }

    std::size_t i = 0;
    uint32_t id = packet.sn;
    while (id != ack_sn && i < packet.data.size()) {
      i++;
      id++;
    }
    std::size_t flushed = 0;
    try {
      flushed = recv_queue.try_send_lock_held(&packet.data[0] + i, packet.data.size() - i);
      ack_sn += flushed;
      #ifdef AU_DEBUG
      if (flushed) {
        std::cout << "Received " << packet.data.size() << " bytes; flushed " << flushed << " of them; till " << ack_sn << "\n";
      }
      #endif
    } catch (const locking_queue_shut_down&) {
    }
    if (flushed || send_window_changed) {
      send_some_data(flushed ? Flags::ACK : Flags::NONE);
    }
  } else {
    std::cerr << "Received some packet from " << packet.source << " to " << packet.dest
              << "; data_len=" << packet.data.size()
              << "; flags=" << static_cast<int>(packet.flags)
              << "\n";
  }
}

void connection_impl::send_some_data(Flags flags) {
  static thread_local char buf[MAX_SEGMENT_SIZE];
  std::size_t size = 0;
  auto &send_window_queue = send_window.queue_lock_held();
  // Here we heavily exploit the fact that cyclic_queue cannot contain elements
  // will all possible IDs, hence begin_id != end_id in non-empty cyclic_queue.
  for (uint32_t id = send_window_queue.begin_id(); id != send_window_queue.end_id() && size < sizeof(buf); id++) {
    buf[size] = send_window_queue[id];
    size++;
  }

  if (flags != Flags::NONE || size > 0) {
    uint32_t begin_id = send_window_queue.begin_id();
    std::vector<char> data(buf, buf + size);

    #ifdef AU_DEBUG
    std::cout << "Sending packet with flags=" << static_cast<int>(flags) << ", data starts from " << send_window_queue.begin_id() << " and goes for " << data.size << "\n";
    #endif
    send_packet(flags, begin_id, data);

    retrier_.retry_after(SEND_ACK_TIMEOUT, [begin_id, flags, data, this]() {
      std::lock_guard<std::mutex> lock(mutex_);
      auto &send_window_queue_ = send_window.queue_lock_held();
      if (send_window_queue_.begin_id() != begin_id) {
        return true;
      }
      #ifdef AU_DEBUG
      std::cout << "Sending packet with flags=" << static_cast<int>(flags) << ", data starts from " << send_window_queue.begin_id() << " and goes for " << data.size() << "\n";
      #endif
      send_packet(flags, begin_id, data);
      return false;
    });
  }
}

void connection_impl::send(const char *buf, size_t size) {
  while (size > 0) {
    std::size_t sent;
    try {
      sent = send_window.try_send_or_block(buf, size);
    } catch (const locking_queue_shut_down&) {
      throw socket_io_error("Connection terminated");
    }
    buf += sent;
    size -= sent;
    std::lock_guard<std::mutex> lock(mutex_);
    send_some_data(Flags::NONE);
  }
}

void connection_impl::recv(char *buf, size_t size) {
  try {
    recv_queue.recv(buf, size);
  } catch (const locking_queue_shut_down&) {
    throw socket_eof_error("EOF");
  }
}

void connection_impl::shutdown() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ <= ESTABLISHED) {
      send_packet(Flags::FIN, send_window.queue_lock_held().begin_id(), {});
      // TODO: add retry after timeout
      state_ = FIN_SENT;
    }
  }
  // Wait for real shut down
  for (;;) {
    char buf[1];
    try {
      recv_queue.recv(buf, 1);
    } catch (const locking_queue_shut_down&) {
      break;
    }
  }
  // TODO: add shutdown after timeout
}

}  // namespace au_stream_socket
