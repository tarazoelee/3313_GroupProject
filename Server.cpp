#include "thread.h"

#include "socketserver.h"

#include <stdlib.h>

#include <time.h>

#include <list>

#include <vector>

#include <algorithm>

#include "socket.h"

#include <iostream>

#include <mutex>

using namespace Sync;

// This thread handles the connection to the server
class ClientThread : public Thread
{
private:
  // Reference to our connected socket
  Socket &socket;

  // Reference to boolean flag for terminating the thread
  bool &terminate;

  // Are we connected?
  bool connected = false;

  // Data to send to server
  ByteArray data;
  std::string data_str;
  int expectedLength = 0;

public:
  ClientThread(Socket &socket, bool &terminate)
      : socket(socket), terminate(terminate)
  {
  }

  ~ClientThread()
  {
  }

  void TryConnect()
  {
    try
    {
      std::cout << "Connecting...";
      std::cout.flush();
      int result = socket.Open();
      connected = true;
      std::cout << "OK" << std::endl;
    }
    catch (std::string exception)
    {
      std::cout << "FAIL (" << exception << ")" << std::endl;
      return;
    }
  }

  virtual long ThreadMain()
  {
    // Initially we need a connection
    while (true)
    {
      // Attempt to connect
      TryConnect();

      // Check if we are exiting or connection was established
      if (terminate || connected)
      {
        break;
      }

      // Try again every 5 seconds
      std::cout << "Trying again in 5 seconds" << std::endl;
      sleep(5);
    }

    while (!terminate)
    {
      // We are connected, perform our operations
      std::cout << "Please input your data (done to exit): ";
      std::cout.flush();

      // Get the data
      data_str.clear();
      std::getline(std::cin, data_str);
      data = ByteArray(data_str);
      expectedLength = data_str.size();

      // Must have data to send
      if (expectedLength == 0)
      {
        std::cout << "Cannot send no data!" << std::endl;
        continue;
      }
      else if (data_str == "done")
      {
        std::cout << "Closing the client..." << std::endl;
        terminate = true;
        break;
      }

      // Write to the server
      socket.Write(data);

      // Get the response
      socket.Read(data);
      data_str = data.ToString();

      // If the length is different, the socket must be closed
      if (data_str.size() != expectedLength)
      {
        std::cout << "Server failed to respond. Closing client..." << std::endl;
        terminate = true;
      }
      else
      {
        std::cout << "Server Response: " << data_str << std::endl;
      }

      // Are we terminating
      if (terminate)
      {
        break;
      }
    }

    return 0;
  }
};

// extends Thread class. Responsible for handling client connections.
class SocketThread : public Thread
{
private:
  ByteArray data;

  bool &killThread;             // creating reference to killThread
  std::list<Socket *> &sockets; // list of connected sockets

public:
  Socket &socket;

  SocketThread(Socket &socket, bool &killThread, std::list<Socket *> &sockets)
      : socket(socket), killThread(killThread), sockets(sockets) {}

  ~SocketThread() {}

  Socket &GetSocket()
  {
    return socket;
  }

  virtual long ThreadMain()
  {

    while (socket.Read(data) > 0)
    { // if socket is not closed, read data that is sent back if data exists
      std::string response = data.ToString();

      for (Socket *otherSocket : sockets)
      { // send back response to other sockets
        if (otherSocket != &socket)
        {
          otherSocket->Write(response);
        }
      }
      // if (response == "CLOSE")
      // {
      //   // Close the socket and remove it from the list of connected sockets
      //   for (auto socket : sockets)
      //   {
      //     try
      //     {
      //       socket->Close(); // assuming Close() is a member function of the Socket class that closes the socket
      //       sockets.remove(socket);
      //     }
      //     catch (...)
      //     {
      //       // ignore errors when closing sockets
      //     }
      //   }

      //   // Clear the list of sockets
      //   sockets.clear();
      //   break;
      // }
    }

    // Close the socket and remove it from the list of connected sockets when killThread is set to true
    // socket.Close();
    // sockets.remove(&socket);
    // delete this;
    // return 0;
  }
};

// defines the ServerThread class that extends Thread class, it is responsible for handling server operations
class ServerThread : public Thread
{
private:
  SocketServer &server;
  std::vector<SocketThread *> sockThreads;
  std::list<Socket *> sockets; // list of connected sockets
  bool killThread = false;
  std::mutex mutex; // mutex to protect access to killThread

public:
  ServerThread(SocketServer &server, std::vector<SocketThread *> sockThreads) : server(server),
                                                                                sockThreads(sockThreads) {}

  ~ServerThread()
  {
    // Close the client sockets
    for (auto thread : sockThreads)
    {
      try
      {
        // close the sockets
        Socket &toClose = thread->GetSocket();
        toClose.Close();
      }
      catch (...)
      {
        // if already ended will throw an error
        std::cout << "erroring" << std::endl;
        /// killThread = true;
      }
    }
    // TERMINATE THE THREAD LOOPS
    {
      // Lock the mutex to protect access to killThread
      std::lock_guard<std::mutex> lock(mutex);
      killThread = true;
    }
  }

  virtual long ThreadMain()
  {
    while (true)
    {
      try
      {
        // Wait for a client socket connection
        while (sockets.size() < 2)
        {
          Socket *newConnection = new Socket(server.Accept());
          sockets.push_back(newConnection); // add new socket to list of connected sockets

          // Pass a reference to this pointer into a new socket thread
          Socket &socketReference = *newConnection;
          sockThreads.push_back(new SocketThread(socketReference, killThread, sockets)); // pass list of sockets to new SocketThread instance
        }

        server.Shutdown(); // NEED THIS OR ELSE GET BAD FILE DESCRIPTOR
      }
      catch (const std::exception &ex)
      {
        std::cerr << "Caught exception: " << ex.what() << std::endl;
      }
      catch (const std::basic_string<char> &str)
      {
        std::cerr << "Caught basic_string exception: " << str << std::endl;
      }
      catch (...)
      {
        std::cerr << "Caught unknown exception" << std::endl;
      }
    }
  }
};

int main(void)
{

  try
  {
    std::string userInput = " ";
    std::string createOrJoin = " ";
    int joinPort = 0;
    std::cout << "Would you like to create (C) a match or join a match (J)" << std::endl;
    std::cout.flush();
    std::cin >> createOrJoin;

    if (createOrJoin == "C")
    {
      std::srand(static_cast<unsigned int>(std::time(nullptr)));
      int serverPort = std::rand() % (65535 - 1024 + 1) + 1024;
      int player1Score = 0;
      int player2Score = 0;
      SocketServer server(serverPort);
      std::cout << "Your Match Code: " + std::to_string(serverPort) << std::endl;
      std::vector<SocketThread *> sockThreads;

      // creating instance of serverThread class, with server and socketThread vector for this instance
      ServerThread serverThread(server, sockThreads);
      Socket socket("127.0.0.1", serverPort);

      if (socket.Open())
      {
        int gamesPlayed = 0;
        ByteArray alteredMessage;

        while (gamesPlayed < 5)
        {
          std::string choice = " ";
          std::cout << "Write your choice of Rock, Paper, or Scissors. Write CLOSE to close the game." << std::endl;
          std::cin >> choice;

          socket.Write(ByteArray(choice)); // keep this here so that doesn't write to the socket if closed

          // reads the return message from the Server
          socket.Read(alteredMessage);
          std::string opponentChoice = alteredMessage.ToString();

          std::cout << "Opponent wrote: " << opponentChoice << " You wrote: " << choice << std::endl;

          if (opponentChoice == "Rock" && choice == "Paper" || opponentChoice == "Paper" && choice == "Scissors" || opponentChoice == "Scissors" && choice == "Rock")
          {
            player1Score++;
            std::cout << "-----------------------" << std::endl;
            std::cout << "You Win!" << std::endl;
            std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
            std::cout << "-----------------------" << std::endl;
          }
          else if (opponentChoice == "Rock" && choice == "Scissors" || opponentChoice == "Paper" && choice == "Rock" || opponentChoice == "Scissors" && choice == "Paper")
          {
            player2Score++;
            std::cout << "-----------------------" << std::endl;
            std::cout << "You Lose!" << std::endl;
            std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
            std::cout << "-----------------------" << std::endl;
          }
          else if (opponentChoice == "Rock" && choice == "Rock" || opponentChoice == "Paper" && choice == "Paper" || opponentChoice == "Scissors" && choice == "Scissors")
          {
            std::cout << "-----------------------" << std::endl;
            std::cout << "It's a tie!" << std::endl;
            std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
            std::cout << "-----------------------" << std::endl;
          }
          else
          {
            std::cout << "-----------------------" << std::endl;
            std::cout << "!! Invalid input, please try again !!" << std::endl;
            std::cout << "-----------------------" << std::endl;
          }

          gamesPlayed++;
        }
        std::cout << "GAME OVER" << std::endl;
        try
        {
          socket.Write(ByteArray("CLOSE"));
        }
        catch (...)
        {
        }
      }
    }

    else if (createOrJoin == "J")
    {
      std::cout << "Enter Match Code:" << std::endl;
      std::cin >> joinPort;
      Socket socket("127.0.0.1", joinPort);
      int player1Score = 0;
      int player2Score = 0;

      if (socket.Open())
      {
        int gamesPlayed = 0;
        while (gamesPlayed < 5)
        {
          std::string choice = " ";
          std::cout << "Write your choice of Rock, Paper, or Scissors. Write CLOSE to close the game." << std::endl;
          std::cin >> choice;

          socket.Write(ByteArray(choice));
          if (choice == "CLOSE")
          {
            break;
          }

          ByteArray alteredMessage;

          socket.Read(alteredMessage); // reads the return message from the Server
          std::string opponentChoice = alteredMessage.ToString();
          if (opponentChoice == "CLOSE")
          {
            socket.Close();
            break;
          }
          std::cout << "Opponent wrote: " << opponentChoice << " You wrote: " << choice << std::endl;

          if (opponentChoice == "Rock" && choice == "Paper" || opponentChoice == "Paper" && choice == "Scissors" || opponentChoice == "Scissors" && choice == "Rock")
          {
            player1Score++;
            std::cout << "-----------------------" << std::endl;
            std::cout << "You Win!" << std::endl;
            std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
            std::cout << "-----------------------" << std::endl;
          }
          else if (opponentChoice == "Rock" && choice == "Scissors" || opponentChoice == "Paper" && choice == "Rock" || opponentChoice == "Scissors" && choice == "Paper")
          {
            player2Score++;
            std::cout << "-----------------------" << std::endl;
            std::cout << "You Lose!" << std::endl;
            std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
            std::cout << "-----------------------" << std::endl;
          }
          else if (opponentChoice == "Rock" && choice == "Rock" || opponentChoice == "Paper" && choice == "Paper" || opponentChoice == "Scissors" && choice == "Scissors")
          {
            std::cout << "-----------------------" << std::endl;
            std::cout << "It's a tie!" << std::endl;
            std::cout << "Opponent score: " << player2Score << ", Your score: " << player1Score << std::endl;
            std::cout << "-----------------------" << std::endl;
          }
          else
          {
            std::cout << "-----------------------" << std::endl;
            std::cout << "!! Invalid input, please try again !!" << std::endl;
            std::cout << "-----------------------" << std::endl;
          }
          gamesPlayed++;
        }
        std::cout << "GAME OVER" << std::endl;
        try
        {
          socket.Write(ByteArray("CLOSE"));
        }
        catch (...)
        {
        }
      }
    }
    else
    {
      std::cout << "Invalid input" << std::endl;
    }
  }
  catch (...)
  {
    std::cerr << "An error occurred. Please try again later." << std::endl;
  }
}