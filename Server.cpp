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
class SocketThread : public Thread {
private:
  ByteArray data;

  bool& killThread; // creating reference to killThread
  std::list<Socket*>& sockets; // list of connected sockets

public:
  Socket& socket;

  SocketThread(Socket& socket, bool& killThread, std::list<Socket*>& sockets)
    : socket(socket), killThread(killThread), sockets(sockets) {}

  ~SocketThread() {}

  Socket& GetSocket() {
    return socket;
  }

  virtual long ThreadMain() {
   
      
        while (socket.Read(data) > 0) { // if socket is not closed, read data that is sent back if data exists
          std::string response = data.ToString();


          for (Socket* otherSocket : sockets) { // send back response to other sockets
            if (otherSocket != &socket) {
              otherSocket->Write(response);
            }
          }
          if (response == "CLOSE") {
            // Close the socket and remove it from the list of connected sockets
           for (auto socket : sockets) {
            try {
              socket->Close(); // assuming Close() is a member function of the Socket class that closes the socket
              sockets.remove(socket);
                } catch (...) {
                 // ignore errors when closing sockets
                 }   
            }

          // Clear the list of sockets
          sockets.clear();
          break;
          } 
      

          
        }
    
    

    // Close the socket and remove it from the list of connected sockets when killThread is set to true
    // socket.Close();
    // sockets.remove(&socket);
    // delete this;
    // return 0;
  }
};



//defines the ServerThread class that extends Thread class, it is responsible for handling server operations
class ServerThread: public Thread {
  private: 
    SocketServer & server;
    std::vector < SocketThread * > sockThreads;
    std::list < Socket * > sockets; // list of connected sockets
    bool killThread = false;
  public: 
    ServerThread(SocketServer & server, std::vector < SocketThread * > sockThreads): server(server),
    sockThreads(sockThreads) {}

  ~ServerThread() {
    // Close the client sockets
    for (auto thread: sockThreads) {
      try {
        // close the sockets
        Socket& toClose = thread->GetSocket();
        toClose.Close();
      } 
      catch (...) {
        //if already ended will throw an error 
         std::cout << "erroring" << std::endl;
         ///killThread = true;
      }
    }
    //TERMINATE THE THREAD LOOPS
    killThread = true;
  }

  virtual long ThreadMain() {
    while (true) {
      try {
          // Wait for a client socket connection
          while(sockets.size() < 2) {
              Socket * newConnection = new Socket(server.Accept());
              sockets.push_back(newConnection); // add new socket to list of connected sockets

              // Pass a reference to this pointer into a new socket thread
              Socket & socketReference = * newConnection;
              sockThreads.push_back(new SocketThread(socketReference, killThread, sockets)); // pass list of sockets to new SocketThread instance
          }

    server.Shutdown(); //NEED THIS OR ELSE GET BAD FILE DESCRIPTOR 

    } catch (const std::exception & ex) {
        std::cerr << "Caught exception: " << ex.what() << std::endl;
    } catch (const std::basic_string<char>& str) {
        std::cerr << "Caught basic_string exception: " << str << std::endl;
    } catch (...) {
        std::cerr << "Caught unknown exception" << std::endl;
    }
    }
  }
};



int main(void) {

try{
  std::string userInput = " ";
  std::string createOrJoin = " ";
  int joinPort = 0;
  std::cout << "Would you like to create (C) a match or join a match (J)" << std::endl;
  std::cout.flush();
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
      ByteArray alteredMessage;

      while (gamesPlayed < 5) {
        std::string choice = " ";
        std::cout << "Write your choice of Rock, Paper, or Scissors. Write CLOSE to close the game." << std::endl;
        std::cin >> choice;
      
           /*
          int result = shutdown(socket, 0);
          if (result == 0) {
                // shutdown successful
                 std::cout << "Closed successfully." << std::endl;
            } else {
                // handle error
                 std::cout << "ERROR" << std::endl;
            }
          socket.Close();
          server.Shutdown();
           */
          // break;
      
        socket.Write(ByteArray(choice)); //keep this here so that doesn't write to the socket if closed 

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
        } 
        else if (opponentChoice == "Rock" && choice == "Scissors" || opponentChoice == "Paper" && choice == "Rock" || opponentChoice == "Scissors" && choice == "Paper") {
          player2Score++;
          std::cout << "-----------------------" << std::endl;
          std::cout << "You Lose!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
          std::cout << "-----------------------" << std::endl;
        } 
        else if (opponentChoice == "Rock" && choice == "Rock" || opponentChoice == "Paper" && choice == "Paper" || opponentChoice == "Scissors" && choice == "Scissors") {
          std::cout << "-----------------------" << std::endl;
          std::cout << "It's a tie!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
          std::cout << "-----------------------" << std::endl;
        }
        else {
          std::cout << "-----------------------" << std::endl;
          std::cout << "!! Invalid input, please try again !!" << std::endl;
          std::cout << "-----------------------" << std::endl;
        }

        gamesPlayed++;

      }
    }
  } 

  else if (createOrJoin == "J") {
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

      socket.Write(ByteArray(choice));
       if (choice == "CLOSE"){
          socket.Close();
          break;
        }

      ByteArray alteredMessage;

      socket.Read(alteredMessage); //reads the return message from the Server
      std::string opponentChoice = alteredMessage.ToString();
      if (opponentChoice == "CLOSE"){
socket.Close();
          break;
      }
      std::cout << "Opponent wrote: " << opponentChoice << " You wrote: " << choice << std::endl;

      if (opponentChoice == "Rock" && choice == "Paper" || opponentChoice == "Paper" && choice == "Scissors" || opponentChoice == "Scissors" && choice == "Rock") {
          player1Score++;
          std::cout << "-----------------------" << std::endl;
          std::cout << "You Win!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
          std::cout << "-----------------------" << std::endl;
      } 
      else if (opponentChoice == "Rock" && choice == "Scissors" || opponentChoice == "Paper" && choice == "Rock" || opponentChoice == "Scissors" && choice == "Paper") {
          player2Score++;
          std::cout << "-----------------------" << std::endl;
          std::cout << "You Lose!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
          std::cout << "-----------------------" << std::endl;
      } 
      else if (opponentChoice == "Rock" && choice == "Rock" || opponentChoice == "Paper" && choice == "Paper" || opponentChoice == "Scissors" && choice == "Scissors") {
          std::cout << "-----------------------" << std::endl;
          std::cout << "It's a tie!" << std::endl;
          std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
          std::cout << "-----------------------" << std::endl;
      }
      else {
          std::cout << "-----------------------" << std::endl;
          std::cout << "!! Invalid input, please try again !!" << std::endl;
          std::cout << "-----------------------" << std::endl;
        }
        gamesPlayed++;
     }
    }

    //FlexWait cinWaiter(1, stdin); //Wait for input to shutdown the server

    } 
    else {
      std::cout << "Invalid input" << std::endl;
    }

  }
    catch(...){
        std::cerr << "An error occurred. Please try again later." << std::endl;
    }

}