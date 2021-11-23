#pragma once

#include "Player.h"
#include "Projectile.h"

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
		player->roll = *(bff + 8);

	}

	//Takes 40 bytes
	static void CopyProjectileMovementData(Projectile* projectile, char* bff)
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

	static void ReadProjectileMovementData(Projectile* projectile, char* buffer)
	{
		float* bff = (float*)buffer;

		float x, y, z, pitch, yaw, roll;

		x = *bff;
		y = *(bff + 1);
		z = *(bff + 2);
		projectile->GetTransform()->SetPosition(x, y, z);

		projectile->velocityX = *(bff + 3);
		projectile->velocityY = *(bff + 4);
		projectile->velocityZ = *(bff + 5);

		pitch = *(bff + 6);
		yaw = *(bff + 7);
		roll = *(bff + 8);
		projectile->GetTransform()->SetRotation(pitch, yaw, roll);

		projectile->gravity = *(bff + 9);
		projectile->lifespan = *(bff + 10);
		projectile->age = *(bff + 11);

		projectile->dead = false;

	}

};

