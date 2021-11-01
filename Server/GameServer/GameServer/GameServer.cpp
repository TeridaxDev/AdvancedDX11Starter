// GameServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include "../../../Network.h"

int main()
{
    std::string IP = "127.0.0.1";
    int PORT = 8888;

    try
    {
        WSASession Session;
        UDPSocket Socket;
        std::string data = "hello world";
        char buffer[100];
        Socket.Bind(PORT);

        std::cout << "Server online. Port number " << PORT << std::endl;

        while (1)
        {
            Socket.RecvFrom(&buffer[0], 100);
            std::cout << buffer << std::endl;
        }
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what();
    }
}

