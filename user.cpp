#include <SFML/Network.hpp>
#include <thread>
#include <iostream>

std::string username;

void handleInput(sf::TcpSocket* socket, std::atomic<bool>& running) { 
  while(running) {

    std::string message;
    std::cout << " > " << std::flush;
    std::getline(std::cin, message);

    if (message == "/leave") { 
      running = false;
      socket->disconnect();
      break;
    }

    sf::Packet packet;
    packet << message;

    if (socket->send(packet) != sf::Socket::Done) {
      std::cerr << "Failed to send message." << std::endl;
      break;
    }
  }
}

void receiveMessage(sf::TcpSocket* socket, std::atomic<bool>& running) {
  sf::Packet packet;
  while(running) {
    sf::Socket::Status status = socket->receive(packet);

    if (status == sf::Socket::Done) {
      std::string message;

      if (packet >> message) {
        static std::mutex coutMutex;
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "\n" << message << std::endl;
        std::cout << " > " << std::flush;
      }
      packet.clear();
    }
    else {
      std::cerr << "\nDisconnected from server." << std::endl;
      running = false;
      break;
    } 
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: ./build/user <server-ip>" << std::endl;
    return 1;
  }

  std::string server_ip = argv[1];

  sf::TcpSocket* socket = new sf::TcpSocket;
  sf::Socket::Status status = socket->connect(server_ip, 53000);

  if (status != sf::Socket::Done) {
    std::cerr << "Failed to connect to the server." << std::endl;
    delete socket;
    return 1; 
  }

  std::cout << "Username: ";
  std::getline(std::cin, username);
  if (username.empty()) username = "user";

  sf::Packet packet;
  packet << username;
  if (socket->send(packet) != sf::Socket::Done) {
    std::cerr << "Failed to send username to server." << std::endl;
    delete socket;
    return 1;
  }

  std::cout << "Welcome, " << username << "!" << std::endl;
  std::cout << "Use command '/leave' to exit!" << std::endl;

  std::atomic<bool> running(true);

  std::thread inputThread(handleInput, socket, std::ref(running));
  std::thread receiveThread(receiveMessage, socket, std::ref(running));
  
  inputThread.join();
  receiveThread.join();

  delete socket;

  return 0;
}
