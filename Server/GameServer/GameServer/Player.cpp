#include "Player.h"

Player::Player(sockaddr_in sender, int id)
{
	client = sender;
	ID = id;
}
