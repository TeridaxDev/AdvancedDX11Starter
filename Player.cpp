#include "Player.h"

Player::Player(Mesh* mesh, Material* material, Camera* camera) : GameEntity(mesh, material)
{
	Player::camera = camera;
}

void Player::Update(float dt)
{
}
