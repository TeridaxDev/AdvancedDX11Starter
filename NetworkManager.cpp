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

	for (size_t i = 0; i < remotePlayers.size(); i++)
	{
		Player* p = remotePlayers[i];
		if (p == nullptr) continue;

		auto it = find(entities->begin(), entities->end(), p);
		auto it2 = find(remotePlayers.begin(), remotePlayers.end(), p);
		entities->erase(it);
		remotePlayers.erase(it2);
		delete p->GetCamera();
		delete p;
	}

	if (state != NetworkState::Offline)
	{
		Disconnect();
	}
}

NetworkResult NetworkManager::Connect(std::string ip, int port, Player* local, Mesh* mesh, Material* mat)
{
	
	IP = ip;
	PORT = port;

	playerMesh = mesh;
	playerMat = mat;

	state = NetworkState::Connecting;
	socket.Bind(0);

	//Clear buffers
	std::fill_n(sendBuffer, 500, 0);
	std::fill_n(recvBuffer, 500, 0);

	unsigned int msgType = 1;

	std::memcpy(&sendBuffer, &msgType, 4);

	//Send initial position and velocity
	CopyPlayerMovementData(local, &sendBuffer[0] + 4);

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

//Takes 24 Bytes
void NetworkManager::CopyPlayerMovementData(Player* player, char* bff)
{

	float x = player->GetCamera()->GetTransform()->GetPosition().x;
	float y = player->GetCamera()->GetTransform()->GetPosition().y;
	float z = player->GetCamera()->GetTransform()->GetPosition().z;

	//Position x/y/z
	std::memcpy(bff, &x, 4);
	std::memcpy(bff + 4, &y, 4);
	std::memcpy(bff + 8, &z, 4);

	//Velocity x/y/z
	std::memcpy(bff + 12, &player->velocityX, 4);
	std::memcpy(bff + 16, &player->velocityY, 4);
	std::memcpy(bff + 20, &player->velocityZ, 4);
}

void NetworkManager::ReadPlayerMovementData(Player* player, char* buffer)
{
	float* bff = (float*)buffer;

	float posX, posY, posZ, velX, velY, velZ;
	posX = *bff;
	posY = *(bff+1);
	posZ = *(bff+2);
	velX = *(bff+3);
	velY = *(bff+4);
	velZ = *(bff+5);

	player->GetCamera()->GetTransform()->SetPosition(posX, posY, posZ);
	player->SetVelocity(velX, velY, velZ);
}

void NetworkManager::Update(float dt, Player* local)
{

	if (newData)
	{

		//Handle received data
		unsigned int* msgType = (unsigned int*)&recvBuffer;

		if (*msgType == 1) //Connected request accepted
		{
			if (state == NetworkState::Connecting) state = NetworkState::Connected;

			std::cout << "\nJoined as player " << *(msgType + 1) << std::endl;
			playerID = *(msgType + 1);
			//Create the remote players
			for (size_t i = 0; i < *(msgType + 1); i++)
			{
				Player* newPlayer = new Player(playerMesh, playerMat, new Camera(0,10,-5, 3.0f,1.0f, 1280.0f / 720.0f), false);
				newPlayer->GetTransform()->SetPosition(0, -1, 0);
				newPlayer->GetTransform()->SetParent(newPlayer->GetCamera()->GetTransform(), false);
				remotePlayers.push_back(newPlayer);
				entities->push_back(newPlayer);
			}
			remotePlayers.push_back(nullptr); //Reserved spot for the local player

		}
		else if (*msgType == 10 && state == NetworkState::Connected) //Remote Player Update
		{
			for (size_t i = 0; i < remotePlayers.size(); i++)
			{
				if (remotePlayers[i] == nullptr) continue;

				ReadPlayerMovementData(remotePlayers[i], &recvBuffer[0] + 4 + (24 * i));
			}
		}


		//Clear buffer
		std::fill_n(recvBuffer, 500, 0);
		newData = false;
	}


	if (state == NetworkState::Connected)
	{
		for (size_t i = 0; i < remotePlayers.size(); i++)
		{
			if (remotePlayers[i] == nullptr) continue;

			remotePlayers[i]->Update(dt);
		}
	}

}
