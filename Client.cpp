#include "socket.h"
#include "thread.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

using namespace Sync;

int main(void)
{
	std::string userInput = " ";
	std:int port = 0;
	std::cout << "I am a client" << std::endl;
	std::cin >> port;

	Socket socket("127.0.0.1",port);
	

	if(socket.Open()){

		while(userInput != "Exit"){
		
		//asks client to type somthing
		std::cout << "Type a String to be converted to CAPS (Type Exit to close the Client)" << std::endl;
		std::cin >> userInput;

		//writes to the server
		socket.Write(ByteArray(userInput));
		if(userInput != "Exit"){
		ByteArray alteredMessage;

		//reads the return message from the Server
		socket.Read(alteredMessage);

		//closes the socket if the server is closed
		if(alteredMessage.ToString() == ""){
			socket.Close();
			break;
		}
		
		//prints alteredMessage
		std::cout<<"Converted Message: "<< alteredMessage.ToString()<<std::endl;
		}
		}
	}
	socket.Close();
}