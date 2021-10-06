#pragma once
#include "GameEntity.h"
#include "Camera.h"

class Player : public GameEntity
{
private:

	float moveSpeed = 15.0f;
	float gravity = -.98f;
	float jumpForce = 14;
	float mouseLookSpeed = 1.0f;

	float velocityX;
	float velocityY;
	float velocityZ;

	Camera* camera;


	//TEMPORARY
	float floorHeight = -8;

public:

	Player(Mesh* mesh, Material* material, Camera* camera);

	Camera* GetCamera() { return camera; }

	void Update(float dt);

};

