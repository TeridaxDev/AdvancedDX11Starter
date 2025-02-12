#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "GameEntity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "SpriteFont.h"
#include "SpriteBatch.h"
#include "Lights.h"
#include "Sky.h"
#include "Input.h"
#include "Player.h"
#include "Renderer.h"
#include "Projectile.h"
#include "NetworkManager.h"
#include "Emitter.h"

#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <vector>

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Input and mesh swapping
	byte keys[256];
	byte prevKeys[256];

	// Keep track of "stuff" to clean up
	std::vector<Material*> materials;
	std::vector<GameEntity*>* currentScene;
	std::vector<GameEntity*> entities;
	Projectile* projectiles[MAX_PROJECTILES];
	Camera* camera;
	Player* localPlayer;

	// Lights
	std::vector<Light> lights;
	int lightCount;

	//Emitters
	std::vector<Emitter*> emitters;

	// These will be loaded along with other assets and
	// saved to these variables for ease of access
	Mesh* lightMesh;
	SimpleVertexShader* lightVS;
	SimplePixelShader* lightPS;

	// Text & ui
	DirectX::SpriteFont* arial;
	DirectX::SpriteBatch* spriteBatch;

	// Texture related resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSampler;

	// Skybox
	Sky* sky;

	// General helpers for setup and drawing
	void GenerateLights();
	void DrawUI();

	//Renderer
	Renderer* renderer;

	// Initialization helper method
	void LoadAssetsAndCreateEntities();

	Input& input = Input::GetInstance();

	//Networking
	NetworkManager* netManager;

	//Text Fields for ImGui
	char ip[15] = "127.0.0.1";
	char port[6] = "8888";

};

