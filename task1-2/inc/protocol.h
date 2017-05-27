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

typedef std::uint64_t t_client_id;
typedef std::int64_t t_balance;

class MessageVisitor;

struct AbstractMessage {
  virtual ~AbstractMessage() {};
  virtual void serialize(std::ostream &os) const = 0;
  virtual void deserialize(std::istream &is) = 0;
  virtual std::uint8_t id() const = 0;
  virtual std::size_t serialized_size() const = 0;
  virtual void visit(MessageVisitor&) const = 0;
};

struct RegistrationMessage : public AbstractMessage {
  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
  void visit(MessageVisitor&) const override;
};

struct LoginMessage : public AbstractMessage {
  t_client_id client_id;

  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
  void visit(MessageVisitor&) const override;
};

struct RegistrationResponse : public AbstractMessage {
  t_client_id client_id;
  
  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
  void visit(MessageVisitor&) const override;
};

struct BalanceInquiryRequest : public AbstractMessage {
  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
  void visit(MessageVisitor&) const override;
};

struct BalanceInquiryResponse : public AbstractMessage {
  t_balance balance;

  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
  void visit(MessageVisitor&) const override;
};

struct TransferRequest : public AbstractMessage {
  t_client_id transfer_to;
  t_balance amount;

  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
  void visit(MessageVisitor&) const override;
};

struct OperationSucceeded : public AbstractMessage {
  void serialize(std::ostream &os) const override;
  void deserialize(std::istream &is) override;
  std::uint8_t id() const override;
  std::size_t serialized_size() const override;
  void visit(MessageVisitor&) const override;
};

struct MessageVisitor {
  virtual ~MessageVisitor() {};
  virtual void accept(const RegistrationMessage&) = 0;
  virtual void accept(const LoginMessage&) = 0;
  virtual void accept(const RegistrationResponse&) = 0;
  virtual void accept(const BalanceInquiryRequest&) = 0;
  virtual void accept(const BalanceInquiryResponse&) = 0;
  virtual void accept(const TransferRequest&) = 0;
  virtual void accept(const OperationSucceeded&) = 0;
};

std::unique_ptr<AbstractMessage> proto_recv(stream_socket &sock);
void proto_send(stream_socket &sock, const AbstractMessage &msg);

#endif  // PROTOCOL_H_
