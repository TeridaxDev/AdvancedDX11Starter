#pragma once

#include <string>
#include <thread>
#include "Network.h"

enum class NetworkState
{
	Offline,
	Connecting,
	Connected
};

enum class NetworkResult
{
	SUCCESS,
	FAILURE
};

class NetworkManager
{
private:

	std::string IP;
	int PORT;
	WSASession session;
	UDPSocket socket;

	char sendBuffer[500];
	char recvBuffer[500];
	bool newData;
	bool running;

	NetworkState state = NetworkState::Offline;

	std::thread recvFromThread;

	void ReceiveFrom();

public:
	
	~NetworkManager();

	NetworkState GetNetworkState() { return state; }

	NetworkResult Connect(std::string ip, int port);
	NetworkResult Disconnect();

	void Update(float dt);

};

