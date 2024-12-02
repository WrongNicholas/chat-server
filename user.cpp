#include <SFML/Network.hpp>
#include <ncurses.h>

#include <thread>
#include <iostream>
#include <mutex>
#include <atomic>
#include <deque>

std::string username;
std::mutex messageMutex;
std::deque<std::string> chatMessages;

int MESSAGE_AREA = 30;
const int INPUT_AREA_HEIGHT = 3;

void updateMessageWindow(WINDOW* messageWindow) {
  std::lock_guard<std::mutex> lock(messageMutex);
  werase(messageWindow); 

  int line = 1;

  for (const auto& message : chatMessages) {
    if (line >= MESSAGE_AREA) break;
    mvwprintw(messageWindow, line++, 1, "%s", message.c_str());
  }

  box(messageWindow, 0, 0);
  wrefresh(messageWindow);
}

void handleInput(sf::TcpSocket* socket, WINDOW* inputWindow, WINDOW* messageWindow, std::atomic<bool>& running) { 
  while(running) {
    werase(inputWindow);
    box(inputWindow, 0, 0);
    mvwprintw(inputWindow, 1, 1, " > ");
    wmove(inputWindow, 1, 4);
    wrefresh(inputWindow);

    char inputBuffer[256] = {0};
    wgetnstr(inputWindow, inputBuffer, sizeof(inputBuffer) - 1);

    std::string message(inputBuffer);
    if (message == "/leave") { 
      running = false;
      socket->disconnect();
      break;
    }

    if (message.empty()) {
      continue;
    }

    sf::Packet packet;
    packet << message;

    if (socket->send(packet) != sf::Socket::Done) {
      std::cerr << "Failed to send message." << std::endl;
      break;
    }

    {
      std::lock_guard<std::mutex> lock(messageMutex);
      chatMessages.push_back("[" + username + "] " + message);

      while (chatMessages.size() >= MESSAGE_AREA) {
        chatMessages.pop_front();
      }
    }

    updateMessageWindow(messageWindow);
  }
}

void receiveMessage(sf::TcpSocket* socket, WINDOW* messageWindow, std::atomic<bool>& running) {
  sf::Packet packet;
  while(running) {
    sf::Socket::Status status = socket->receive(packet);

    if (status == sf::Socket::Done) {
      std::string message;

      if (packet >> message) {

        {
          std::lock_guard<std::mutex> lock(messageMutex);
          chatMessages.push_back(message);

          while (chatMessages.size() >= MESSAGE_AREA) {
            chatMessages.pop_front();
          }
        }
        
        updateMessageWindow(messageWindow);

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

  initscr();
  cbreak();

  MESSAGE_AREA = LINES - 3;

  WINDOW* messageWindow = newwin(MESSAGE_AREA, COLS, 0, 0);
  WINDOW* inputWindow = newwin(INPUT_AREA_HEIGHT, COLS, MESSAGE_AREA, 0);

  scrollok(messageWindow, TRUE);

  box(messageWindow, 0, 0);
  box(inputWindow, 0, 0);
  wrefresh(messageWindow);
  wrefresh(inputWindow);

  std::string welcomeMessage = "Welcome, " + username + "! Use command '/leave' to exit.";
  {
    std::lock_guard<std::mutex> lock(messageMutex);
    chatMessages.push_front(welcomeMessage);
  }
  updateMessageWindow(messageWindow);

  std::atomic<bool> running(true);

  std::thread inputThread(handleInput, socket, inputWindow, messageWindow, std::ref(running));
  std::thread receiveThread(receiveMessage, socket, messageWindow, std::ref(running)); 

  inputThread.join();
  receiveThread.join();

  delwin(messageWindow);
  delwin(inputWindow);
  endwin();
  delete socket;

  return 0;
}
