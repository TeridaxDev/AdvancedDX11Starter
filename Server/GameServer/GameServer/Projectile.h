#pragma once
class Projectile
{

public:

	float positionX;
	float positionY;
	float positionZ;

	float velocityX;
	float velocityY;
	float velocityZ;

	float pitch;
	float yaw;
	float roll;

	float gravity;
	float lifespan;
	float age;

	bool dead = false;

	void Update(float dt);

};

