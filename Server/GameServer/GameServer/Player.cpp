#include "Player.h"

Player::Player(sockaddr_in sender, int id)
{
	client = sender;
	ID = id;
}

void Player::SetPosition(float x, float y, float z)
{
	positionX = x;
	positionY = y;
	positionZ = z;
}

void Player::SetVelocity(float x, float y, float z)
{
	velocityX = x;
	velocityY = y;
	velocityZ = z;
}

void Player::Update(float dt)
{
}
