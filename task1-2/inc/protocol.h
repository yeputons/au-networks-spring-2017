#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdint.h>
#include <iostream>
#include <memory>
#include <exception>
#include <string>
#include "stream_socket.h"

class protocol_error : public std::runtime_error {
public:
  protocol_error(const std::string &what_arg) : std::runtime_error(what_arg) {}
};

struct AbstractMessage {
  virtual ~AbstractMessage() {};
  virtual void serialize(std::ostream &os) const = 0;
  virtual void deserialize(std::istream &is) = 0;
  virtual std::uint8_t id() const = 0;
  virtual std::size_t serialized_size() const = 0;
};

struct RegistrationMessage : public AbstractMessage {
  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
};

struct LoginMessage : public AbstractMessage {
  std::uint64_t client_id;

  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
};

struct RegistrationResponse : public AbstractMessage {
  std::uint64_t client_id;
  
  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
};

struct BalanceInquiryRequest : public AbstractMessage {
  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
};

struct BalanceInquiryResponse : public AbstractMessage {
  std::int64_t balance;

  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
};

struct TransferRequest : public AbstractMessage {
  std::uint64_t transfer_to;
  std::int64_t amount;

  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
};

std::unique_ptr<AbstractMessage> proto_recv(stream_socket &sock);
void proto_send(stream_socket &sock, const AbstractMessage &msg);

#endif  // PROTOCOL_H_
