#pragma once

#include "../../../Network.h"

class Player
{
private:
	//Constants
	float moveSpeed = 15.0f;
	float gravity = -19.6f;
	float jumpForce = 8;
	float mouseLookSpeed = 1.0f;
	
	int ID;
	sockaddr_in client;

	float positionX;
	float positionY;
	float positionZ;
	
	float velocityX;
	float velocityY;
	float velocityZ;


public:

	Player(sockaddr_in sender, int id);

	int GetID() { return ID; }

};

