#include "Projectile.h"

Projectile::Projectile(Mesh* mesh, Material* material, float lifespan) : GameEntity(mesh, material)
{

	Projectile::lifespan = lifespan;

	velocityX = 0;
	velocityY = 0;
	velocityZ = 0;
	age = 0;

}

void Projectile::SetVelocity(float x, float y, float z, float g)
{
	velocityX = x;
	velocityY = y;
	velocityZ = z;
	gravity = g;
}

void Projectile::Update(float dt)
{
	age += dt;

	if (age >= lifespan) dead = true;

	velocityY += gravity * dt;
	transform.MoveRelative(velocityX * dt, velocityY * dt, velocityZ * dt);

	if (transform.GetPosition().y <= -6) dead = true;

}
