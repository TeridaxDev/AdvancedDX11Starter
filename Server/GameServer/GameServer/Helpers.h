#pragma once

#include "Player.h"

static class Helpers
{
public:
	static void CopyPlayerMovementData(Player* player, void* buffer)
	{
		char* bff = (char*)buffer;

		//Position x/y/z
		std::memcpy(bff, &player->positionX, 4);
		std::memcpy(bff + 4, &player->positionY, 4);
		std::memcpy(bff + 8, &player->positionZ, 4);

		//Velocity x/y/z
		std::memcpy(bff + 12, &player->velocityX, 4);
		std::memcpy(bff + 16, &player->velocityY, 4);
		std::memcpy(bff + 20, &player->velocityZ, 4);
	}

};

