#include "NetworkManager.h"
#include <bitset>

void NetworkManager::ReceiveFrom()
{
	while (running)
	{
		while (!newData)
		{
			try 
			{
				socket.RecvFrom(recvBuffer, 500);
			}
			catch (std::exception& ex)
			{
				std::cout << "RecvFrom error." << std::endl;
			}
			newData = true;
		}
	}
}

NetworkManager::~NetworkManager()
{
	if (state != NetworkState::Offline)
	{
		Disconnect();
	}
}

NetworkResult NetworkManager::Connect(std::string ip, int port)
{
	
	IP = ip;
	PORT = port;

	state = NetworkState::Connecting;
	socket.Bind(0);

	//Clear buffers
	std::fill_n(sendBuffer, 500, 0);
	std::fill_n(recvBuffer, 500, 0);

	unsigned int msgType = 1;

	std::memcpy(&sendBuffer, &msgType, 4);

	socket.SendTo(IP, PORT, sendBuffer, 500);

	running = true;
	recvFromThread = std::thread(&NetworkManager::ReceiveFrom, this);

	return NetworkResult::SUCCESS;
}

NetworkResult NetworkManager::Disconnect()
{
	IP = "";
	PORT = 0;
	running = false;
	newData = false;
	//session.~WSASession();
	socket.~UDPSocket();
	recvFromThread.join();
	socket = UDPSocket();
	//session = WSASession();
	state = NetworkState::Offline;

	return NetworkResult::SUCCESS;
}

void NetworkManager::Update(float dt)
{

	if (newData)
	{
		//std::cout << recvBuffer << std::endl;

		//Handle received data
		unsigned int* msgType = (unsigned int*)&recvBuffer;

		if (*msgType == 1) //Connected request accepted
		{
			if (state == NetworkState::Connecting) state = NetworkState::Connected;

			unsigned int* msgData = (unsigned int*)&recvBuffer[4];

			for (size_t i = 0; i < 8; i++)
			{
				std::bitset<8> x(recvBuffer[i]);
				std::cout << x << " ";
			}

			std::cout << "\nJoined as player " << *msgData << std::endl;

		}



		//Clear buffer
		std::fill_n(recvBuffer, 500, 0);
		newData = false;
	}

}
