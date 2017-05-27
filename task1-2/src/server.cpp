#include <assert.h>
#include <memory>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <map>
#include "protocol.h"
#include "tcp_socket.h"

std::map<t_client_id, t_balance> balances;
std::mutex balances_mutex;

t_client_id register_new_client() {
  std::lock_guard<std::mutex> lock(balances_mutex);
  t_client_id id = balances.size();
  assert(!balances.count(id));
  balances[id] = 0;
  return id;
}

t_balance get_amount(t_client_id id) {
  std::lock_guard<std::mutex> lock(balances_mutex);
  auto it = balances.find(id);
  if (it == balances.end()) {
    throw std::runtime_error("Requested balance for an unknown client");
  }
  return it->second;
}

void transfer(t_client_id from, t_client_id to, t_balance amount) {
  std::lock_guard<std::mutex> lock(balances_mutex);
  auto it_from = balances.find(from);
  auto it_to = balances.find(to);
  if (it_from == balances.end() || it_to == balances.end()) {
    throw std::runtime_error("Requested transfer for an unknown client");
  }
  it_from->second -= amount;
  it_to->second += amount;
}

class ClientHandler : public MessageVisitor {
public:
  ClientHandler(stream_socket *sock) : sock_(sock), client_id_(-1) {}

  void accept(const RegistrationMessage&) {
    std::cout << "Received RegistrationMessage()" << std::endl;
    client_id_ = register_new_client();

    RegistrationResponse resp;
    resp.client_id = client_id_;
    proto_send(*sock_, resp);
  }

  void accept(const LoginMessage &m) {
    std::cout << "Received LoginMessage(client_id=" << m.client_id << ")" << std::endl;
    client_id_ = m.client_id;
    get_amount(client_id_);  // Check that client exists.
    proto_send(*sock_, OperationSucceeded());
  }

  void accept(const RegistrationResponse&) {
    throw std::runtime_error("Unexpected RegistrationResponse");
  }

  void accept(const BalanceInquiryRequest&) {
    std::cout << "Received BalanceInquiryRequest()" << std::endl;

    BalanceInquiryResponse resp;
    resp.balance = get_amount(client_id_);
    proto_send(*sock_, resp);
  }

  void accept(const BalanceInquiryResponse&) {
    throw std::runtime_error("Unexpected BalanceInquiryResponse");
  }

  void accept(const TransferRequest &m) {
    std::cout << "Received TransferRequest("
              << "to=" << m.transfer_to << ", "
              << "amount=" << m.amount << ")" << std::endl;
    transfer(client_id_, m.transfer_to, m.amount);
    proto_send(*sock_, OperationSucceeded());
  }

  void accept(const OperationSucceeded&) {
    throw std::runtime_error("Unexpected OperationSucceeded");
  }

private:
  stream_socket *sock_;
  std::uint64_t client_id_;
};

void process_client(std::unique_ptr<stream_socket> client) {
  ClientHandler handler(client.get());
  for (;;) {
    try {
      std::unique_ptr<AbstractMessage> msg_gen = proto_recv(*client);
      msg_gen->visit(handler);
    } catch (const std::exception &e) {
      std::cout << "Exception caught while processing client: " << e.what() << std::endl;
      break;
    }
  }
}

int main(int argc, char* argv[]) {
  std::string host = "127.0.0.1";
  int port = 40001;

  if (argc > 1) {
    host = argv[1];
  }
  if (argc > 2) {
    port = atoi(argv[2]);
  }

  try {
    std::cout << "Trying to listen on " << host << ":" << port << "..." << std::endl;
    tcp_server_socket server(host.c_str(), port);
    std::cout << "Listening..." << std::endl;

    for (;;) {
      std::unique_ptr<stream_socket> client(server.accept_one_client());
      std::cout << "New client" << std::endl;
      std::thread th(process_client, std::move(client));
      th.detach();
    }
  } catch (const std::exception &e) {
    std::cout << "Exception caught in the main loop: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
