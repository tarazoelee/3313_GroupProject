#include "thread.h"

  #include "socketserver.h"

  #include <stdlib.h>

  #include <time.h>

  #include <list>

  #include <vector>

  #include <algorithm>

  #include "socket.h"

  #include <iostream>

  using namespace Sync;

//extends Thread class. Responsible for handling client connections.
class SocketThread: public Thread {
  private:
    //variable/obj declartions
    ByteArray data;

  bool & killThread; //creating reference to killThread
  std::list < Socket * > & sockets; // list of connected sockets

  public: Socket & socket;

  SocketThread(Socket & socket, bool & killThread, std::list < Socket * > & sockets): socket(socket),
  killThread(killThread),
  sockets(sockets) {}

  ~SocketThread() {}

  Socket & GetSocket() {
    return socket;
  }

  //Continuosly reads data from the Socket object, converts it to uppercase, and sends it back to the client.
  virtual long ThreadMain() {
      while (!killThread) {
        try {
          if (killThread) {
            socket.Close();
            sockets.remove(&socket);  // remove the socket from the list of connected sockets
            delete this;
          }

          while (socket.Read(data) > 0) {
            std::string response = data.ToString();
            if (response == "EXIT") {
              std::cout << "Client has closed...\n";
              socket.Close();
              sockets.remove(&socket);  // remove the socket from the list of connected sockets
              delete this;
              return 0;  // exit the thread
            }

            // std::for_each(response.begin(), response.end(), [](char& c){
            //   c = ::toupper(c);
            // });

            for (Socket* otherSocket : sockets) {
              if (otherSocket != &socket) {
                otherSocket->Write(response);
              }
            }
          }
        } catch (...) {
          killThread = true;
        }
      }

      return 0;
    }
};

//defines the ServerThread class that extends Thread class, it is responsible for handling server operations
class ServerThread: public Thread {
  private: std::vector < SocketThread * > sockThreads;
  std::list < Socket * > sockets; // list of connected sockets
  bool killThread = false;
  public: SocketServer & server;
  ServerThread(SocketServer & server, std::vector < SocketThread * > sockThreads): server(server),
  sockThreads(sockThreads) {}

  ~ServerThread() {
    //Cleanup threads
    for (auto thread: sockThreads) {
      try {
        // close the sockets
        Socket & toClose = thread -> GetSocket();
        toClose.Close();
        delete thread;
      } catch (...) {
        killThread = true;
      }
    }

    killThread = true;
  }

  virtual long ThreadMain() {
    while (!killThread) {
      try {
        Socket * newConnection = new Socket(server.Accept());
        sockets.push_back(newConnection); // add new socket to list of connected sockets

        Socket & socketReference = * newConnection;
        sockThreads.push_back(new SocketThread(socketReference, killThread, sockets)); // pass list of sockets to new SocketThread instance
      } catch (...) {
        killThread = true;
      }
    }
  }
};

int main(void) {

  std::string userInput = " ";
  std::string createOrJoin = " ";
  int joinPort = 0;
  std::cout << "Would you like to create (C) a match or join a match (J)" << std::endl;
  std::cin >> createOrJoin;

  if (createOrJoin == "C") {
    std::srand(static_cast < unsigned int > (std::time(nullptr)));
    int serverPort = std::rand() % (65535 - 1024 + 1) + 1024;
    int player1Score = 0;
    int player2Score = 0;
    SocketServer server(serverPort);
    std::cout << "Your Match Code: " + std::to_string(serverPort) << std::endl;
    std::vector < SocketThread * > sockThreads;

    //creating instance of serverThread class, with server and socketThread vector for this instance 
    ServerThread serverThread(server, sockThreads);

    Socket socket("127.0.0.1", serverPort);
    if (socket.Open()) {
        int gamesPlayed = 0;
        while (gamesPlayed < 5) {
      std::string choice = " ";
      std::cout << "Write your choice of Rock, Paper, or Scissors. Write CLOSE to close the game." << std::endl;
      std::cin >> choice;

      if (choice == "CLOSE") {
          break;
        }

      socket.Write(ByteArray(choice));

      ByteArray alteredMessage;

      //reads the return message from the Server
      socket.Read(alteredMessage);
      std::string opponentChoice = alteredMessage.ToString();

      std::cout << "Opponent wrote: " << opponentChoice << " You wrote: " << choice << std::endl;

 if (opponentChoice == "Rock" && choice == "Paper" || opponentChoice == "Paper" && choice == "Scissors" || opponentChoice == "Scissors" && choice == "Rock") {
            player1Score++;
             std::cout << "-----------------------" << std::endl;
          std::cout << "You Win!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
          std::cout << "-----------------------" << std::endl;
        } else if (opponentChoice == "Rock" && choice == "Scissors" || opponentChoice == "Paper" && choice == "Rock" || opponentChoice == "Scissors" && choice == "Paper") {
            player2Score++;
             std::cout << "-----------------------" << std::endl;
          std::cout << "You Lose!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
          std::cout << "-----------------------" << std::endl;
        } else if (opponentChoice == "Rock" && choice == "Rock" || opponentChoice == "Paper" && choice == "Paper" || opponentChoice == "Scissors" && choice == "Scissors") {
             std::cout << "-----------------------" << std::endl;
          std::cout << "It's a tie!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
           std::cout << "-----------------------" << std::endl;
        } else {
             std::cout << "-----------------------" << std::endl;
             std::cout << "!! Invalid input, please try again !!" << std::endl;
              std::cout << "-----------------------" << std::endl;
        }
        gamesPlayed++;
        }

    }

    //Wait for input to shutdown the server
    FlexWait cinWaiter(1, stdin);

    while (true) {
      std::string input;
      std::getline(std::cin, input);

      //break if client writes close 
      if (input == "CLOSE") {
        break;
      }
    }

    server.Shutdown();

  } else if (createOrJoin == "J") {
    std::cout << "Enter Match Code:" << std::endl;
    std::cin >> joinPort;
    Socket socket("127.0.0.1", joinPort);
    int player1Score = 0;
    int player2Score = 0;
    if (socket.Open()) {
        int gamesPlayed = 0;
        while (gamesPlayed < 5) {
      std::string choice = " ";
      std::cout << "Write your choice of Rock, Paper, or Scissors. Write CLOSE to close the game." << std::endl;
      std::cin >> choice;

      if (choice == "CLOSE") {
          break;
        }

      socket.Write(ByteArray(choice));

      ByteArray alteredMessage;

      //reads the return message from the Server
      socket.Read(alteredMessage);
      std::string opponentChoice = alteredMessage.ToString();

      std::cout << "Opponent wrote: " << opponentChoice << " You wrote: " << choice << std::endl;

 if (opponentChoice == "Rock" && choice == "Paper" || opponentChoice == "Paper" && choice == "Scissors" || opponentChoice == "Scissors" && choice == "Rock") {
            player1Score++;
             std::cout << "-----------------------" << std::endl;
          std::cout << "You Win!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
          std::cout << "-----------------------" << std::endl;
        } else if (opponentChoice == "Rock" && choice == "Scissors" || opponentChoice == "Paper" && choice == "Rock" || opponentChoice == "Scissors" && choice == "Paper") {
            player2Score++;
             std::cout << "-----------------------" << std::endl;
          std::cout << "You Lose!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
          std::cout << "-----------------------" << std::endl;
        } else if (opponentChoice == "Rock" && choice == "Rock" || opponentChoice == "Paper" && choice == "Paper" || opponentChoice == "Scissors" && choice == "Scissors") {
             std::cout << "-----------------------" << std::endl;
          std::cout << "It's a tie!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
           std::cout << "-----------------------" << std::endl;
        } else {
             std::cout << "-----------------------" << std::endl;
             std::cout << "!! Invalid input, please try again !!" << std::endl;
              std::cout << "-----------------------" << std::endl;
        }
        gamesPlayed++;
        }

    }

     //Wait for input to shutdown the server
    FlexWait cinWaiter(1, stdin);

    while (true) {
      std::string input;
      std::getline(std::cin, input);

      //break if client writes close 
      if (input == "CLOSE") {
        break;
      }
    }

  } else {
    //error
  }

}