#include <SFML/Network.hpp>
#include <thread>
#include <iostream>

struct User {
  sf::TcpSocket* socket;
  std::string username;

  User(sf::TcpSocket* socket, const std::string& username)
    : socket(socket), username(username) { }
};

std::vector<User*> users;
std::mutex usersMutex;

void messageAll(const std::string& message, User* sender) {
  std::lock_guard<std::mutex> lock(usersMutex);

  sf::Packet packet;
  packet << message;

  for (User* user : users) {
    if (user == sender) continue;

    if (user->socket->send(packet) != sf::Socket::Done)
      std::cerr << "Failed to send message to " << user->username << std::endl;
  }
}

void handleUser(User* user) {
  sf::Packet packet;

  while(true) {
    if (user->socket->receive(packet) == sf::Socket::Done) {
      std::string message;
      if (packet >> message) {
        message = "[" + user->username + "] " + message;
        std::cout << message << std::endl;

        messageAll(message, user);

        packet.clear();
      }
    }
    else {
      std::string disconnectMessage = "'" + user->username + "' disconnected.";
      std::cout << disconnectMessage << std::endl;
      messageAll(disconnectMessage, user);

      {
        std::lock_guard<std::mutex> lock(usersMutex);
        auto it = std::find(users.begin(), users.end(), user);
        if (it != users.end()) {
          users.erase(it);
        }
      }

      delete user->socket;
      delete user;
      break;
    }
  }
}

int main() {
  sf::TcpListener listener;

  if (listener.listen(53000) != sf::Socket::Done) {
    // Error
    return 1;
  } 

  listener.setBlocking(false);

  std::vector<std::thread> userThreads;

  while (true) {
    sf::TcpSocket* socket = new sf::TcpSocket;
    if (listener.accept(*socket) == sf::Socket::Done) {

      sf::Packet packet;
      if (socket->receive(packet) == sf::Socket::Done) {
        std::string username;
        if (packet >> username) {
          std::string joinMessage = "'" + username + "' has joined the server.";
          std::cout << joinMessage << std::endl;

          User* newUser = new User(socket, username);

          {
            std::lock_guard<std::mutex> lock(usersMutex);
            users.push_back(newUser);
          }

          messageAll(joinMessage, newUser);

          userThreads.emplace_back(handleUser, newUser);
        }
      }
      else {
        delete socket;
      }
    }
    else {
      sf::sleep(sf::milliseconds(100));
    }
  }

  for (auto& thread : userThreads) {
    if (thread.joinable())
      thread.join();
  }

  for (User* user : users) {
    delete user->socket;
    delete user;
  }
  
  return 0;
}
