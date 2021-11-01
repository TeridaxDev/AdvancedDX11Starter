#pragma once

#include <string>
#include "Network.h"

class NetworkManager
{
private:

	std::string IP;
	int PORT;
	WSASession session;
	UDPSocket socket;

public:



};

