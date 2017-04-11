#include <assert.h>
#include <memory>
#include <thread>
#include <stdexcept>
#include "protocol.h"
#include "tcp_socket.h"

class ClientHandler : public MessageVisitor {
public:
  ClientHandler(stream_socket *sock) : sock_(sock) {}

  void accept(const RegistrationMessage&) {
    std::cout << "Received RegistrationMessage()\n";
  }

  void accept(const LoginMessage &m) {
    std::cout << "Received LoginMessage(client_id=" << m.client_id << ")\n";
  }

  void accept(const RegistrationResponse&) {
    throw std::runtime_error("Unexpected RegistrationResponse");
  }

  void accept(const BalanceInquiryRequest&) {
    std::cout << "Received BalanceInquiryRequest()\n";
  }

  void accept(const BalanceInquiryResponse&) {
    throw std::runtime_error("Unexpected BalanceInquiryResponse");
  }

  void accept(const TransferRequest &m) {
    std::cout << "Received TransferRequest("
              << "to=" << m.transfer_to << ", "
              << "amount=" << m.amount << ")\n";
  }

private:
  stream_socket *sock_;
};

void process_client(std::unique_ptr<stream_socket> client) {
  ClientHandler handler(client.get());
  for (;;) {
    try {
      std::unique_ptr<AbstractMessage> msg_gen = proto_recv(*client);
      msg_gen->visit(handler);
    } catch (const std::exception &e) {
      std::cout << "Exception caught while processing client: " << e.what() << "\n";
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
    std::cout << "Trying to listen on " << host << ":" << port << "...\n";
    tcp_server_socket server(host.c_str(), port);
    std::cout << "Listening...\n";

    for (;;) {
      std::unique_ptr<stream_socket> client(server.accept_one_client());
      std::cout << "New client\n";
      std::thread th(process_client, std::move(client));
      th.detach();
    }
  } catch (const std::exception &e) {
    std::cout << "Exception caught in the main loop: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
