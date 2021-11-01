#include "NetworkManager.h"

void NetworkManager::ReceiveFrom()
{
	while (running)
	{
		while (!newData)
		{
			try 
			{
				socket.RecvFrom(recvBuffer, 100);
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

	socket.SendTo(IP, PORT, "alekgn", 6);

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
		std::cout << recvBuffer << std::endl;
		newData = false;

		if (state == NetworkState::Connecting) state = NetworkState::Connected;

	}

}
