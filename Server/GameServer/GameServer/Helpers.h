#pragma once

#include "Player.h"

static class Helpers
{
public:
	static void CopyPlayerMovementData(Player* player, char* bff)
	{
		//Position x/y/z
		std::memcpy(bff, &player->positionX, 4);
		std::memcpy(bff + 4, &player->positionY, 4);
		std::memcpy(bff + 8, &player->positionZ, 4);

		//Velocity x/y/z
		std::memcpy(bff + 12, &player->velocityX, 4);
		std::memcpy(bff + 16, &player->velocityY, 4);
		std::memcpy(bff + 20, &player->velocityZ, 4);

		//pitch/yaw/roll
		std::memcpy(bff + 24, &player->pitch, 4);
		std::memcpy(bff + 28, &player->yaw, 4);
		std::memcpy(bff + 32, &player->roll, 4);

	}

	static void ReadPlayerMovementData(Player* player, char* buffer)
	{
		float* bff = (float*)buffer;
		player->positionX = *bff;
		player->positionY = *(bff+1);
		player->positionZ = *(bff+2);

		player->velocityX = *(bff + 3);
		player->velocityY = *(bff + 4);
		player->velocityZ = *(bff + 5);
		
		player->pitch = *(bff + 6);
		player->yaw = *(bff + 7);
		player->roll = *(bff + 6);

	}



};

