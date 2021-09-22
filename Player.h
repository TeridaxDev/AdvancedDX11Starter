#pragma once
#include "GameEntity.h"
#include "Camera.h"

class Player : public GameEntity
{
private:

	float moveSpeed = 3.0f;
	float gravity = -9.8f;
	float jumpForce = 50;


	Camera* camera;

public:

	Player(Mesh* mesh, Material* material, Camera* camera);

	Camera* GetCamera() { return camera; }

	void Update(float dt);

};

