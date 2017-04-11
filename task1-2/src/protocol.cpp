#include <assert.h>
#include <sstream>
#include <string>
#include <vector>
#include <type_traits>
#include "protocol.h"

using std::ostream;
using std::istream;
using std::stringstream;

template<typename T>
void write(ostream &os, const T& val_) {
  typename std::make_unsigned<T>::type val = val_;
  for (int i = static_cast<int>(sizeof(val)) - 1; i >= 0; i--) {
    std::uint8_t byte = (val >> (8 * i)) & 0xFF;
    assert(os.write(reinterpret_cast<char*>(&byte), 1));
  }
}

template<typename T>
T read(istream &is) {
  typename std::make_unsigned<T>::type result = 0;
  for (int i = static_cast<int>(sizeof(result)) - 1; i >= 0; i--) {
    std::uint8_t byte;
    if (!is.read(reinterpret_cast<char*>(&byte), sizeof(byte))) {
      throw protocol_error("Unexpected EOF");
    }
    result |= byte << (8 * i);
  }
  return result;
}

void RegistrationMessage::serialize(ostream &) const {}
void RegistrationMessage::deserialize(istream &) {}
std::uint8_t RegistrationMessage::id() const { return 1; }
std::size_t RegistrationMessage::serialized_size() const { return 0; }

void LoginMessage::serialize(ostream &os) const { write(os, client_id); }
void LoginMessage::deserialize(istream &is) { client_id = read<uint64_t>(is); }
std::uint8_t LoginMessage::id() const { return 2; }
std::size_t LoginMessage::serialized_size() const { return sizeof(uint64_t); }

void RegistrationResponse::serialize(ostream &os) const { write(os, client_id); }
void RegistrationResponse::deserialize(istream &is) { client_id = read<uint64_t>(is); }
std::uint8_t RegistrationResponse::id() const { return 3; }
std::size_t RegistrationResponse::serialized_size() const { return sizeof(uint64_t); }

void BalanceInquiryRequest::serialize(ostream &) const {}
void BalanceInquiryRequest::deserialize(istream &) {}
std::uint8_t BalanceInquiryRequest::id() const { return 4; }
std::size_t BalanceInquiryRequest::serialized_size() const { return 0; }

void BalanceInquiryResponse::serialize(ostream &os) const { write(os, balance); }
void BalanceInquiryResponse::deserialize(istream &is) { balance = read<int64_t>(is); }
std::uint8_t BalanceInquiryResponse::id() const { return 5; }
std::size_t BalanceInquiryResponse::serialized_size() const { return sizeof(uint64_t); }

void TransferRequest::serialize(ostream &os) const { write(os, transfer_to); write(os, amount); }
void TransferRequest::deserialize(istream &is) { transfer_to = read<uint64_t>(is); amount = read<int64_t>(is); }
std::uint8_t TransferRequest::id() const { return 6; }
std::size_t TransferRequest::serialized_size() const { return 2 * sizeof(uint64_t); }

std::unique_ptr<AbstractMessage> proto_recv(stream_socket &sock) {
  std::uint8_t id;
  sock.recv(&id, 1);

  std::unique_ptr<AbstractMessage> msg;
  switch (id) {
  case 1: msg.reset(new RegistrationMessage); break;
  case 2: msg.reset(new LoginMessage); break;
  case 3: msg.reset(new RegistrationResponse); break;
  case 4: msg.reset(new BalanceInquiryRequest); break;
  case 5: msg.reset(new BalanceInquiryResponse); break;
  case 6: msg.reset(new TransferRequest); break;
  default:
    stringstream err_msg;
    err_msg << "Unknown message id: " << id;
    throw protocol_error(err_msg.str());
  }
  std::vector<char> data(msg->serialized_size());
  sock.recv(data.data(), data.size());

  stringstream data_stream(std::string(data.begin(), data.end()));
  msg->deserialize(data_stream);
  assert(!data_stream);
  return msg;
}

void proto_send(stream_socket &sock, const AbstractMessage &msg) {
  stringstream data_stream;
  write(data_stream, msg.id());
  msg.serialize(data_stream);

  const auto &data = data_stream.str();
  assert(data.size() == msg.serialized_size());
  sock.send(data.data(), data.size());
}
