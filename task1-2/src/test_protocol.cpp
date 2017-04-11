#include "test.h"
#include "protocol.h"
#include <assert.h>
#include <sstream>
#include <set>

std::set<std::uint8_t> ids;

template<typename T> void fill_message(T&) {
}

template<typename T> void check_message(T&) {
}

template<> void fill_message<LoginMessage>(LoginMessage &msg) {
  msg.client_id = 239017;
}

template<> void check_message<LoginMessage>(LoginMessage &msg) {
  assert(msg.client_id == 239017);
}

template<> void fill_message<RegistrationResponse>(RegistrationResponse &msg) {
  msg.client_id = 239017;
}

template<> void check_message<RegistrationResponse>(RegistrationResponse &msg) {
  assert(msg.client_id == 239017);
}

template<> void fill_message<BalanceInquiryResponse>(BalanceInquiryResponse &msg) {
  msg.balance = 239017;
}

template<> void check_message<BalanceInquiryResponse>(BalanceInquiryResponse &msg) {
  assert(msg.balance == 239017);
}

template<> void fill_message<TransferRequest>(TransferRequest &msg) {
  msg.transfer_to = 239017;
  msg.amount = 17239;
}

template<> void check_message<TransferRequest>(TransferRequest &msg) {
  assert(msg.transfer_to == 239017);
  assert(msg.amount == 17239);
}

template<typename T> void test_message() {
  T msg;
  assert(!ids.count(msg.id()));
  ids.insert(msg.id());
  fill_message(msg);

  std::stringstream sstr;
  msg.serialize(sstr);
  assert(sstr.str().size() == msg.serialized_size());

  T msg_out;
  msg_out.deserialize(sstr);
  check_message(msg_out);
}

void test_protocol() {
  ids.clear();
  test_message<RegistrationMessage>();
  test_message<LoginMessage>();
  test_message<RegistrationResponse>();
  test_message<BalanceInquiryRequest>();
  test_message<BalanceInquiryResponse>();
  test_message<TransferRequest>();
}
