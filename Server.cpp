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
class SocketThread : public Thread
{
private:
    //variable/obj declartions
    ByteArray data;

    bool& killThread; //creating reference to killThread
    std::list<Socket*>& sockets; // list of connected sockets

public:
    Socket& socket;

   SocketThread(Socket& socket, bool& killThread, std::list<Socket*>& sockets)
    : socket(socket), killThread(killThread), sockets(sockets) {}

    ~SocketThread()
    {}

    Socket& GetSocket()
    {
        return socket;
    }

    //Continuosly reads data from the Socket object, converts it to uppercase, and sends it back to the client.
    virtual long ThreadMain()
    {
        //while the client is still open 
        while(!killThread)
        {
            try
            {
                if (killThread) { //check if kill thread is true, if yes then close the socket 
                    socket.Close();
                    delete this;
                }
                while(socket.Read(data) > 0){ //if socket is not closed, read data that is sent back if data exists 
                    std::string response = data.ToString();
                    //convert string to upper case
                   // std::for_each(response.begin(), response.end(), [](char & c){
                    //  c = ::toupper(c);
                    //});
                    

                    //checks if client sent EXIT message, displays that the client is closed on the server side
                    std::string exit = "EXIT";
                    if (response == exit) {
                        std::cout<<"Client has closed...\n";
                    
                    }

                     for (Socket* otherSocket : sockets) {
                        if (otherSocket != &socket) {
                            otherSocket->Write(response); //writing what oyu wrote to the other socket 
                        }
                    }

                    //std::cout<<response<<std::endl;

                }

            }
            catch (...)
            {
                killThread = true; //if error then kill thread 
            }
        }
		
        return 0;
    }
};

//defines the ServerThread class that extends Thread class, it is responsible for handling server operations
class ServerThread : public Thread
{
private:
    std::vector<SocketThread*>  sockThreads;
    std::list<Socket*> sockets; // list of connected sockets
    bool killThread = false;
public:
    SocketServer& server;
    ServerThread(SocketServer& server, std::vector<SocketThread*> sockThreads)
    : server(server), sockThreads(sockThreads)
    {}

    ~ServerThread()
    {
        //Cleanup threads
        for (auto thread : sockThreads)
        {
            try
            {
                // close the sockets
                Socket& toClose = thread->GetSocket();
                toClose.Close();
                delete thread;
            }
            catch (...){
                killThread = true;
            }
        }

        killThread = true;
    }

    virtual long ThreadMain()
    {
        while(!killThread){
            try{
                Socket* newConnection = new Socket(server.Accept());
                sockets.push_back(newConnection); // add new socket to list of connected sockets

                Socket& socketReference = *newConnection;
                sockThreads.push_back(new SocketThread(socketReference, killThread, sockets)); // pass list of sockets to new SocketThread instance
            }
            catch(...)
            {
                killThread = true;
            }
        }
    }
};


int main(void)
{

    std::string userInput = " ";
    std::string createOrJoin = " ";
    int joinPort = 0;
	std::cout << "Would you like to create (C) a match or join a match (J)" << std::endl;
    std::cin >> createOrJoin;

    if (createOrJoin == "C"){
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        int serverPort = std::rand() % (65535 - 1024 + 1) + 1024;
        SocketServer server(serverPort); 
        std::cout << "Your Match Code: " + std::to_string(serverPort) << std::endl;
         std::vector<SocketThread*> sockThreads; 

        //creating instance of serverThread class, with server and socketThread vector for this instance 
        ServerThread serverThread(server, sockThreads);

        Socket socket("127.0.0.1", serverPort);
        if(socket.Open()){
            std:: string test = " ";
            std::cout << "Connected" << std::endl;
            std::cin >> test;
            socket.Write(ByteArray(test));

            ByteArray alteredMessage;

		//reads the return message from the Server
		socket.Read(alteredMessage); 

        std::cout<<"Opponent wrote: "<< alteredMessage.ToString()<<" You wrote: " << test <<std::endl;
        
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
        
    } else if (createOrJoin == "J"){
            std::cout << "Enter Match Code:" << std::endl;  
            std::cin >> joinPort;
            Socket socket("127.0.0.1", joinPort);
            if(socket.Open()){
            std:: string test = " ";
            std::cout << "Connected" << std::endl;
            std::cin >> test;
            socket.Write(ByteArray(test));

            ByteArray alteredMessage;

		    //reads the return message from the Server
		    socket.Read(alteredMessage); 

            std::cout<<"Opponent wrote: "<< alteredMessage.ToString()<<" You wrote: " << test <<std::endl;
            }             
                while (true) {
                std::string input;
                std::getline(std::cin, input);  

        //break if client writes close 
        if (input == "CLOSE") {
        break;
    }
}   
}else {
        //error
}
    
    //Output to client, ask for input and gives direction 
	//std::cout << "Type CLOSE to shutdown the Server \n";
    //std::cout.flush(); //keeps data in memory 
	//std::srand(static_cast<unsigned int>(std::time(nullptr)));

    //creates server
    //int port = std::rand() % (65535 - 1024 + 1) + 1024;
    //SocketServer server(port);   
    //std::cout <<port << std::endl;

    //std::vector<SocketThread*> sockThreads; 

    //creating instance of serverThread class, with server and socketThread vector for this instance 
    //ServerThread serverThread(server, sockThreads);
	
    //Wait for input to shutdown the server
    //FlexWait cinWaiter(1, stdin);

    //turning input into string from client 
   

}