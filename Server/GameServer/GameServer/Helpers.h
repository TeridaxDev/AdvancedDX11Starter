#pragma once

#include <cmath>

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

private:

	//Simulates a couple positions to interpolate collision checking
	static void SimulatePositions(Projectile* projectile, float timeStep, float* x, float* y, float* z)
	{
		float x1, y1, z1, p, w, r;
		float vX, vY, vZ;
		x1 = projectile->GetTransform()->GetPosition().x;
		y1 = projectile->GetTransform()->GetPosition().y;
		z1 = projectile->GetTransform()->GetPosition().z;
		vX = projectile->velocityX;
		vY = projectile->velocityY;
		vZ = projectile->velocityZ;
		p = projectile->GetTransform()->GetPitchYawRoll().x;
		w = projectile->GetTransform()->GetPitchYawRoll().y;
		r = projectile->GetTransform()->GetPitchYawRoll().z;

		vY += projectile->gravity * timeStep;
		Transform tf = Transform();
		tf.SetPosition(x1, y1, z1);
		tf.SetRotation(p, w, r);
		tf.MoveRelative(vX * timeStep, vY * timeStep, vZ * timeStep);

		*x = tf.GetPosition().x;
		*y = tf.GetPosition().y;
		*z = tf.GetPosition().z;

	}

	static float GetDistanceBetweenPoints(float x1, float x2, float y1, float y2, float z1, float z2)
	{
		return sqrt(
			pow(x1 - x2, 2) +
			pow(y1 - y2, 2) +
			pow(z1 - z2, 2));
	}

public:

	//Checks for collision between a projectile and player. Also simulates sever positions between current pos and next frame pos
	static bool CheckProjectileCollision(Player* player, Projectile* projectile, float deltaTime, int simulations = 3)
	{
		//Projectile radius - .1? Diameter .2?
		//Player Radius 1.0

		float x1, x2, y1, y2, z1, z2;
		x1 = player->positionX;
		y1 = player->positionY - 1; //adj. camera height
		z1 = player->positionZ;

		x2 = projectile->GetTransform()->GetPosition().x;
		y2 = projectile->GetTransform()->GetPosition().y;
		z2 = projectile->GetTransform()->GetPosition().z;

		float result = GetDistanceBetweenPoints(x1, x2, y1, y2, z1, z2);

		//Don't bother interping if we collided this frame
		if (result <= 1.1f)
			return true;
		else if (result > 10)
			return false;

		float timestep = deltaTime / (float)simulations; //I think we simulate 1 step past the next frame but i don't care
		float x3, y3, z3;

		for (int i = 1; i <= simulations; i++)
		{
			SimulatePositions(projectile, timestep * i, &x3, &y3, &z3);
			float rs = GetDistanceBetweenPoints(x1, x3, y1, y3, z1, z3);
			if (rs < result) result = rs;
		}

		if (result <= 1.1f) return true;
		return false;

	}

};

