#include "Projectile.h"

#include <DirectXMath.h>

using namespace DirectX;

void Projectile::Update(float dt)
{
	age += dt;

	if (age >= lifespan) dead = true;

	velocityY += gravity * dt;

	//transform.MoveRelative(velocityX * dt, velocityY * dt, velocityZ * dt);

	float moveX, moveY, moveZ;
	moveX = velocityX * dt;
	moveY = velocityY * dt;
	moveZ = velocityZ * dt;

	XMVECTOR movement = XMVectorSet(moveX, moveY, moveZ, 0);
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);

	XMVECTOR dir = XMVector3Rotate(movement, rotQuat);

	XMFLOAT3 pos = XMFLOAT3(positionX, positionY, positionZ);
	XMStoreFloat3(&pos, XMLoadFloat3(&pos) + dir);

	positionX = pos.x;
	positionY = pos.x;
	positionZ = pos.y;

}
