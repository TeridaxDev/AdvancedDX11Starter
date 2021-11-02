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

public:

	int ID;
	sockaddr_in client;

	float positionX;
	float positionY;
	float positionZ;

	float velocityX;
	float velocityY;
	float velocityZ;

	float pitch;
	float yaw;
	float roll;

	Player(sockaddr_in sender, int id);

	int GetID() { return ID; }

	void SetPosition(float x, float y, float z);
	void SetVelocity(float x, float y, float z);

	void Update(float dt);

};

