#include "Player.h"
#include "Input.h"

Player::Player(Mesh* mesh, Material* material, Camera* camera) : GameEntity(mesh, material)
{
	Player::camera = camera;
}

void Player::Update(float dt)
{

	float speed = moveSpeed;

	// Get the input manager instance
	Input& input = Input::GetInstance();

	// Speed up or down as necessary
	if (input.KeyDown(VK_SHIFT)) { speed *= 1.6f; }
	if (input.KeyDown(VK_CONTROL)) { speed *= 0.5f; }

	float x = camera->GetTransform()->GetPosition().x;
	float y = camera->GetTransform()->GetPosition().y;
	float z = camera->GetTransform()->GetPosition().z;

	// Movement
	if (input.KeyDown('W')) { velocityZ = speed; }
	else if (input.KeyDown('S')) { velocityZ = -speed; }
	else velocityZ = 0;
	if (input.KeyDown('A')) { velocityX = -speed; }
	else if (input.KeyDown('D')) { velocityX = speed; }
	else velocityX = 0;
	//if (input.KeyDown('X')) { camera->GetTransform()->MoveAbsolute(0, -speed, 0); }

	// Handle mouse movement only when button is down
	if (input.MouseLeftDown())
	{
		// Calculate cursor change
		float xDiff = dt * mouseLookSpeed * input.GetMouseXDelta();
		float yDiff = dt * mouseLookSpeed * input.GetMouseYDelta();
		camera->GetTransform()->Rotate(yDiff, xDiff, 0);
	}

	//Movement

	if (y > floorHeight)
	{
		velocityY += gravity;
	}
	else if (y <= floorHeight)
	{
		velocityY = 0;
		y = floorHeight;
	}

	//Jump
	if (input.KeyDown(' ') && y <= floorHeight) { velocityY = jumpForce; }

	camera->GetTransform()->MoveRelative(velocityX * dt, 0, velocityZ * dt);
	x = camera->GetTransform()->GetPosition().x;
	z = camera->GetTransform()->GetPosition().z;
	camera->GetTransform()->SetPosition(x, y + velocityY * dt, z);

}
