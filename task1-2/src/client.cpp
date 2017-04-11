#include <assert.h>
#include <iostream>
#include <limits>
#include <string>
#include "protocol.h"
#include "tcp_socket.h"

void help() {
  std::cout << "Available commands:\n"
            << "  register - register as a new client\n"
            << "  login <id> - login as an existing client\n"
            << "  balance - request current balance\n"
            << "  transfer <id> <amount> - transfer <amount> to another client <id>\n";
}

void do_register(stream_client_socket &sock) {
  proto_send(sock, RegistrationMessage());
  auto msg_ptr = proto_recv(sock);
  auto &msg = dynamic_cast<RegistrationResponse&>(*msg_ptr);
  std::cout << "Your client id is " << msg.client_id << ".\n";
}

void do_login(stream_client_socket &sock) {
  LoginMessage msg;
  assert(std::cin >> msg.client_id);
  proto_send(sock, msg);
}

void do_balance(stream_client_socket &sock) {
  proto_send(sock, BalanceInquiryRequest());
  auto msg_ptr = proto_recv(sock);
  auto &msg = dynamic_cast<BalanceInquiryResponse&>(*msg_ptr);
  std::cout << "Your balance is " << msg.balance << ".\n";
}

void do_transfer(stream_client_socket &sock) {
  TransferRequest msg;
  assert(std::cin >> msg.transfer_to);
  assert(std::cin >> msg.amount);
  proto_send(sock, msg);
}

void work(stream_client_socket &sock) {
  for (;;) {
    std::cout << ">>> ";

    std::string command;
    std::cin >> command;
    if (command == "help") {
      help();
    } else if (command == "register") {
      do_register(sock);
    } else if (command == "login") {
      do_login(sock);
    } else if (command == "balance") {
      do_balance(sock);
    } else if (command == "transfer") {
      do_transfer(sock);
    } else {
      std::cout << "Unknown command, type 'help' to get help.\n";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
    std::cout << "Trying to connect on " << host << ":" << port << "...\n";
    tcp_client_socket sock(host.c_str(), port);
    sock.connect();
    std::cout << "Connected.\n";

    work(sock);
  } catch (const std::exception &e) {
    std::cout << "Exception caught: " << e.what() << "\n";
    return 1;
  }
}
