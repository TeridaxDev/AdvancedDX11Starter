#pragma once

#include "../../../Transform.h"

class Projectile
{
private:

	Transform transform;

public:

	Transform* GetTransform() { return &transform; }

	float velocityX;
	float velocityY;
	float velocityZ;

	float gravity;
	float lifespan;
	float age;

	bool dead = false;

	void Update(float dt);

};

