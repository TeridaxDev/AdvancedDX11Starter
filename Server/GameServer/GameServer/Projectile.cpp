#include "Projectile.h"

#include <DirectXMath.h>

using namespace DirectX;

void Projectile::Update(float dt)
{
	age += dt;

	if (age >= lifespan) dead = true;

	velocityY += gravity * dt;
	transform.MoveRelative(velocityX * dt, velocityY * dt, velocityZ * dt);

}
