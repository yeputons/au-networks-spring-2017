#include "test.h"
#include "stream_socket.h"
#include "protocol.h"
#include <assert.h>
#include <sstream>
#include <set>

class stringstream_socket : public stream_socket {
public:
  void send(const void *buf, size_t size) override {
    assert(data_.write(reinterpret_cast<const char*>(buf), size));
  }
  void recv(void *buf, size_t size) override {
    assert(data_.read(reinterpret_cast<char*>(buf), size));
  }
  const std::stringstream& data() const {
    return data_;
  }

private:
  std::stringstream data_;
};

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
  msg.balance = -239017;
}

template<> void check_message<BalanceInquiryResponse>(BalanceInquiryResponse &msg) {
  assert(msg.balance == -239017);
}

template<> void fill_message<TransferRequest>(TransferRequest &msg) {
  msg.transfer_to = 239017;
  msg.amount = -17239;
}

template<> void check_message<TransferRequest>(TransferRequest &msg) {
  assert(msg.transfer_to == 239017);
  assert(msg.amount == -17239);
}

template<typename T> void test_message() {
  std::stringstream sstr;
  stringstream_socket sock;

  {
    T msg;
    assert(!ids.count(msg.id()));
    ids.insert(msg.id());
    fill_message(msg);

    msg.serialize(sstr);
    assert(sstr.str().size() == msg.serialized_size());

    proto_send(sock, msg);
  }

  {
    T msg_out;
    msg_out.deserialize(sstr);
    assert(sstr.rdbuf()->in_avail() == 0);
    check_message(msg_out);
  }
  {
    auto msg_out_ptr = proto_recv(sock);
    assert(sock.data().rdbuf()->in_avail() == 0);
    T &msg_out = dynamic_cast<T&>(*msg_out_ptr);
    check_message(msg_out);
  }
}

void test_protocol() {
  ids.clear();
  test_message<RegistrationMessage>();
  test_message<LoginMessage>();
  test_message<RegistrationResponse>();
  test_message<BalanceInquiryRequest>();
  test_message<BalanceInquiryResponse>();
  test_message<TransferRequest>();
  test_message<OperationSucceeded>();
}
