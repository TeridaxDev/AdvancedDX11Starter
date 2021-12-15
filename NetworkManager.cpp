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
				running = false;
				Disconnect();
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
	try
	{
		socket.Bind(0);
	}
	catch (std::exception& ex)
	{

	}

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
	std::fill_n(sendBuffer, 500, 0);

	unsigned int msgType = 4;
	std::memcpy(&sendBuffer, &msgType, 4);
	msgType = playerID;
	std::memcpy(&sendBuffer[0] + 4, &msgType, 4);
	socket.SendTo(IP, PORT, sendBuffer, 500);

	IP = "";
	PORT = 0;
	running = false;
	newData = true;
	//session.~WSASession();
	recvFromThread.join();
	//socket.~UDPSocket();
	//socket = UDPSocket();
	//session = WSASession();
	state = NetworkState::Offline;

	return NetworkResult::SUCCESS;
}

//Takes 36 Bytes
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

	//Rotation pitch/yaw/roll

	float pitch = player->GetCamera()->GetTransform()->GetPitchYawRoll().x;
	float yaw = player->GetCamera()->GetTransform()->GetPitchYawRoll().y;
	float roll = player->GetCamera()->GetTransform()->GetPitchYawRoll().z;

	std::memcpy(bff + 24, &pitch, 4);
	std::memcpy(bff + 28, &yaw, 4);
	std::memcpy(bff + 32, &roll, 4);
}

void NetworkManager::ReadPlayerMovementData(Player* player, char* buffer)
{
	float* bff = (float*)buffer;

	float posX, posY, posZ, velX, velY, velZ, pitch, yaw, roll;
	posX = *bff;
	posY = *(bff+1);
	posZ = *(bff+2);
	velX = *(bff+3);
	velY = *(bff+4);
	velZ = *(bff+5);
	pitch = *(bff + 6);
	yaw = *(bff + 7);
	roll = *(bff + 8);

	player->GetCamera()->GetTransform()->SetPosition(posX, posY, posZ);
	player->SetVelocity(velX, velY, velZ);
	player->GetCamera()->GetTransform()->SetRotation(pitch, yaw, roll);
}

//Takes 40 bytes
void NetworkManager::CopyProjectileMovementData(Projectile* projectile, char* bff)
{
	float x = projectile->GetTransform()->GetPosition().x;
	float y = projectile->GetTransform()->GetPosition().y;
	float z = projectile->GetTransform()->GetPosition().z;

	//Position x/y/z
	std::memcpy(bff, &x, 4);
	std::memcpy(bff + 4, &y, 4);
	std::memcpy(bff + 8, &z, 4);

	//Velocity x/y/z
	std::memcpy(bff + 12, &projectile->velocityX, 4);
	std::memcpy(bff + 16, &projectile->velocityY, 4);
	std::memcpy(bff + 20, &projectile->velocityZ, 4);

	//Rotation pitch/yaw/roll

	float pitch = projectile->GetTransform()->GetPitchYawRoll().x;
	float yaw = projectile->GetTransform()->GetPitchYawRoll().y;
	float roll = projectile->GetTransform()->GetPitchYawRoll().z;
	float gravity = projectile->gravity;

	std::memcpy(bff + 24, &pitch, 4);
	std::memcpy(bff + 28, &yaw, 4);
	std::memcpy(bff + 32, &roll, 4);
	std::memcpy(bff + 36, &gravity, 4);

	//Age
	float lifespan = projectile->lifespan;
	float age = projectile->age;

	std::memcpy(bff + 40, &lifespan, 4);
	std::memcpy(bff + 44, &age, 4);

}

void NetworkManager::ReadProjectileMovementData(Projectile* projectile, char* buffer)
{
	float* bff = (float*)buffer;

	float posX, posY, posZ, velX, velY, velZ, pitch, yaw, roll, grav, lifespan, age;
	posX = *bff;
	posY = *(bff + 1);
	posZ = *(bff + 2);
	velX = *(bff + 3);
	velY = *(bff + 4);
	velZ = *(bff + 5);
	pitch = *(bff + 6);
	yaw = *(bff + 7);
	roll = *(bff + 8);
	grav = *(bff + 9);
	lifespan = *(bff + 10);
	age = *(bff + 11);

	projectile->GetTransform()->SetPosition(posX, posY, posZ);
	projectile->SetVelocity(velX, velY, velZ, grav);
	projectile->GetTransform()->SetRotation(pitch, yaw, roll);
	projectile->lifespan = lifespan;
	projectile->age = age;
}

void NetworkManager::AddNetworkProjectile(Projectile* projectile, int index)
{
	//Clear buffers
	std::fill_n(sendBuffer, 500, 0);

	unsigned int msgType = 3;
	int ind = index;

	std::memcpy(&sendBuffer, &msgType, 4);
	std::memcpy(&sendBuffer[0] + 4, &ind, 4);

	//Send initial position and velocity
	CopyProjectileMovementData(projectile, &sendBuffer[0] + 8);

	socket.SendTo(IP, PORT, sendBuffer, 500);

}

void NetworkManager::Update(float dt, Player* local, Projectile** projectiles)
{

	while (newData)
	{

		//Handle received data
		unsigned int* msgType = (unsigned int*)&recvBuffer;

		switch (*msgType)
		{
		case 1: //Connected request accepted
		{
			if (state == NetworkState::Connecting) state = NetworkState::Connected;

			std::cout << "\nJoined as player " << *(msgType + 1) << std::endl;
			playerID = *(msgType + 1);
			//Create the remote players'
			if(remotePlayers.size() != *(msgType + 2))
				for (size_t i = 0; i < *(msgType + 2); i++)
				{
					if (i == playerID)
					{
						remotePlayers.push_back(nullptr); //Reserved spot for the local player
						continue;
					}
					Player* newPlayer = new Player(playerMesh, playerMat, new Camera(0, 10, -5, 3.0f, 1.0f, 1280.0f / 720.0f), false);
					newPlayer->GetTransform()->SetPosition(0, -1, 0);
					newPlayer->GetTransform()->SetScale(2, 2, 2);
					newPlayer->GetTransform()->SetParent(newPlayer->GetCamera()->GetTransform(), false);
					remotePlayers.push_back(newPlayer);
					entities->push_back(newPlayer);
				}

		}
		break;
		case 2:
			if (state == NetworkState::Connected) //Player joined
			{
				Player* newPlayer = new Player(playerMesh, playerMat, new Camera(0, 10, -5, 3.0f, 1.0f, 1280.0f / 720.0f), false);
				newPlayer->GetTransform()->SetPosition(0, -1, 0);
				newPlayer->GetTransform()->SetScale(2, 2, 2);
				newPlayer->GetTransform()->SetParent(newPlayer->GetCamera()->GetTransform(), false);
				remotePlayers.push_back(newPlayer);
				entities->push_back(newPlayer);
			}
			break;
			//else if (*msgType == 3 && state == NetworkState::Connected) //New projectile
			//{
			//	return; // Ignore call for now
			//	Projectile* newProjectile = new Projectile(playerMesh, playerMat, 5);
			//	entities->push_back(newProjectile);
			//	newProjectile->GetTransform()->SetScale(0.2f, 0.2f, 0.2f);
			//}
		case 10:
			if (state == NetworkState::Connected) //Remote Player Update
			{
				for (size_t i = 0; i < remotePlayers.size(); i++)
				{
					if (remotePlayers[i] == nullptr) continue;

					ReadPlayerMovementData(remotePlayers[i], &recvBuffer[0] + 4 + (36 * i));
				}
				for (size_t i = 0; i < MAX_PROJECTILES; i++)
				{
					ReadProjectileMovementData(*(projectiles + i), &recvBuffer[0] + 4 + (36 * remotePlayers.size()) + (48 * i));
					Projectile* p = *(projectiles + i);
					if (p->dead && p->age < p->lifespan) p->dead = false; //Fix for when the server resurrects a projectile and doesn't tell us
				}
			}
			break;
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


		//Send current player stats

		//Clear buffers
		std::fill_n(sendBuffer, 500, 0);

		unsigned int msgType = 10;

		std::memcpy(&sendBuffer, &msgType, 4);
		msgType = playerID;
		std::memcpy(&sendBuffer[0] + 4, &msgType, 4); //send player ID so the server can identify us

		//Send current position and velocity
		CopyPlayerMovementData(local, &sendBuffer[0] + 8);

		socket.SendTo(IP, PORT, sendBuffer, 500);

	}

}
