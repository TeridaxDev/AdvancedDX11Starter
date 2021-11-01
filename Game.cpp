
#include <stdlib.h>     // For seeding random and rand()
#include <time.h>       // For grabbing time (to seed random)

#include "Game.h"
#include "Vertex.h"
#include "Input.h"

#include "WICTextureLoader.h"

#include "Extensions/imgui/imgui.h"
#include "Extensions/imgui/backends/imgui_impl_dx11.h"
#include "Extensions/imgui/backends/imgui_impl_win32.h"

#include "AssetLoader.h"

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// Helper macro for getting a float between min and max
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

// Helper macros for making texture and shader loading code more succinct
#define LoadTexture(file, srv) CreateWICTextureFromFile(device.Get(), context.Get(), GetFullPathTo_Wide(file).c_str(), 0, srv.GetAddressOf())
#define LoadShader(type, file) new type(device.Get(), context.Get(), GetFullPathTo_Wide(file).c_str())


// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{
	camera = 0;

	// Seed random
	srand((unsigned int)time(0));

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object

	// Clean up our other resources
	for (auto& m : materials) delete m;
	for (auto& e : entities) delete e;

	// Delete any one-off objects
	delete sky;
	delete camera;
	delete localPlayer; //COMMENT OUT IF RENDERING PLAYER'S BODY
	delete arial;
	delete spriteBatch;

	// Delete singletons
	delete& Input::GetInstance();
	delete& Assets::GetInstance();

	// ImGui cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete renderer;
	delete netManager;

}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Initialize the input manager with the window's handle
	Input::GetInstance().Initialize(this->hWnd);

	// Asset loading and entity creation
	LoadAssetsAndCreateEntities();
	
	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set up lights initially
	lightCount = 64;
	GenerateLights();

	// Make our camera
	camera = new Camera(
		0, 10, -5,	// Position
		3.0f,		// Move speed
		1.0f,		// Mouse look
		this->width / (float)this->height); // Aspect ratio

	localPlayer = new Player(Assets::GetInstance().GetMesh("Models\\LEGO_Man.obj"), materials[0], camera);
	localPlayer->GetTransform()->SetPosition(0, -1, 0);
	localPlayer->GetTransform()->SetParent(camera->GetTransform(), false);
	//entities.push_back(localPlayer); //UNCOMMENT TO RENDER THE PLAYER'S BODY. DONT FORGET TO COMMENT OUT "delete localPlayer"

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());

	renderer = new Renderer(device, context, swapChain, backBufferRTV, depthStencilView, width, height, sky, entities, lights);
	netManager = new NetworkManager();

}


// --------------------------------------------------------
// Load all assets and create materials, entities, etc.
// --------------------------------------------------------
void Game::LoadAssetsAndCreateEntities()
{

	Assets& assets = Assets::GetInstance();
	assets.Initialize("..\\..\\Assets\\", device, context);
	assets.LoadAllAssets();


	// Describe and create our sampler state
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, samplerOptions.GetAddressOf());

	// Also create a clamp sampler
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	device->CreateSamplerState(&sampDesc, clampSampler.GetAddressOf());


	// Create the sky
	sky = new Sky(
		/*assets.GetTexture("Skies\\SunnyCubeMap.dds"),*/
		assets.GetTexture("Skies\\Clouds Blue\\right.png"),
		assets.GetTexture("Skies\\Clouds Blue\\left.png"),
		assets.GetTexture("Skies\\Clouds Blue\\up.png"),
		assets.GetTexture("Skies\\Clouds Blue\\down.png"),
		assets.GetTexture("Skies\\Clouds Blue\\front.png"),
		assets.GetTexture("Skies\\Clouds Blue\\back.png"),
		samplerOptions,
		device,
		context);

	// Grab basic shaders for all these materials
	SimpleVertexShader* vs = assets.GetVertexShader("VertexShader.cso");
	SimplePixelShader* ps = assets.GetPixelShader("PixelShader.cso");
	SimplePixelShader* psPBR = assets.GetPixelShader("PixelShaderPBR.cso");

	lightMesh = assets.GetMesh("Models\\sphere.obj");
	lightVS = vs;
	lightPS = assets.GetPixelShader("SolidColorPS.cso");

	// Create basic materials
	Material* cobbleMat2x = new Material(vs, ps, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	cobbleMat2x->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\cobblestone_albedo.png"));
	cobbleMat2x->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\cobblestone_normals.png"));
	cobbleMat2x->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\cobblestone_roughness.png"));
	cobbleMat2x->AddPSSampler("BasicSampler", samplerOptions);

	Material* floorMat = new Material(vs, ps, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	floorMat->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\floor_albedo.png"));
	floorMat->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\floor_normals.png"));
	floorMat->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\floor_roughness.png"));
	floorMat->AddPSSampler("BasicSampler", samplerOptions);

	Material* paintMat = new Material(vs, ps, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	paintMat->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\paint_albedo.png"));
	paintMat->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\paint_normals.png"));
	paintMat->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\paint_roughness.png"));
	paintMat->AddPSSampler("BasicSampler", samplerOptions);

	Material* scratchedMat = new Material(vs, ps, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	scratchedMat->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\scratched_albedo.png"));
	scratchedMat->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\scratched_normals.png"));
	scratchedMat->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\scratched_roughness.png"));
	scratchedMat->AddPSSampler("BasicSampler", samplerOptions);

	Material* bronzeMat = new Material(vs, ps, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	bronzeMat->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\bronze_albedo.png"));
	bronzeMat->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\bronze_normals.png"));
	bronzeMat->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\bronze_roughness.png"));
	bronzeMat->AddPSSampler("BasicSampler", samplerOptions);

	Material* roughMat = new Material(vs, ps, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	roughMat->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\rough_albedo.png"));
	roughMat->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\rough_normals.png"));
	roughMat->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\rough_roughness.png"));
	roughMat->AddPSSampler("BasicSampler", samplerOptions);

	Material* woodMat = new Material(vs, ps, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	woodMat->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\wood_albedo.png"));
	woodMat->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\wood_normals.png"));
	woodMat->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\wood_roughness.png"));
	woodMat->AddPSSampler("BasicSampler", samplerOptions);


	materials.push_back(cobbleMat2x);
	materials.push_back(floorMat);
	materials.push_back(paintMat);
	materials.push_back(scratchedMat);
	materials.push_back(bronzeMat);
	materials.push_back(roughMat);
	materials.push_back(woodMat);

	// Create PBR materials
	Material* cobbleMat2xPBR = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	cobbleMat2xPBR->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\cobblestone_albedo.png"));
	cobbleMat2xPBR->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\cobblestone_normals.png"));
	cobbleMat2xPBR->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\cobblestone_roughness.png"));
	cobbleMat2xPBR->AddPSTextureSRV("MetalTexture", assets.GetTexture("Textures\\cobblestone_metal.png"));
	cobbleMat2xPBR->AddPSSampler("BasicSampler", samplerOptions);
	cobbleMat2xPBR->AddPSSampler("ClampSampler", clampSampler);

	Material* floorMatPBR = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	floorMatPBR->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\floor_albedo.png"));
	floorMatPBR->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\floor_normals.png"));
	floorMatPBR->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\floor_roughness.png"));
	floorMatPBR->AddPSTextureSRV("MetalTexture", assets.GetTexture("Textures\\floor_metal.png"));
	floorMatPBR->AddPSSampler("BasicSampler", samplerOptions);
	floorMatPBR->AddPSSampler("ClampSampler", clampSampler);

	Material* paintMatPBR = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	paintMatPBR->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\paint_albedo.png"));
	paintMatPBR->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\paint_normals.png"));
	paintMatPBR->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\paint_roughness.png"));
	paintMatPBR->AddPSTextureSRV("MetalTexture", assets.GetTexture("Textures\\paint_metal.png"));
	paintMatPBR->AddPSSampler("BasicSampler", samplerOptions);
	paintMatPBR->AddPSSampler("ClampSampler", clampSampler);

	Material* scratchedMatPBR = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	scratchedMatPBR->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\scratched_albedo.png"));
	scratchedMatPBR->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\scratched_normals.png"));
	scratchedMatPBR->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\scratched_roughness.png"));
	scratchedMatPBR->AddPSTextureSRV("MetalTexture", assets.GetTexture("Textures\\scratched_metal.png"));
	scratchedMatPBR->AddPSSampler("BasicSampler", samplerOptions);
	scratchedMatPBR->AddPSSampler("ClampSampler", clampSampler);

	Material* bronzeMatPBR = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	bronzeMatPBR->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\bronze_albedo.png"));
	bronzeMatPBR->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\bronze_normals.png"));
	bronzeMatPBR->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\bronze_roughness.png"));
	bronzeMatPBR->AddPSTextureSRV("MetalTexture", assets.GetTexture("Textures\\bronze_metal.png"));
	bronzeMatPBR->AddPSSampler("BasicSampler", samplerOptions);
	bronzeMatPBR->AddPSSampler("ClampSampler", clampSampler);

	Material* roughMatPBR = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	roughMatPBR->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\rough_albedo.png"));
	roughMatPBR->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\rough_normals.png"));
	roughMatPBR->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\rough_roughness.png"));
	roughMatPBR->AddPSTextureSRV("MetalTexture", assets.GetTexture("Textures\\rough_metal.png"));
	roughMatPBR->AddPSSampler("BasicSampler", samplerOptions);
	roughMatPBR->AddPSSampler("ClampSampler", clampSampler);

	Material* woodMatPBR = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, XMFLOAT2(2, 2));
	woodMatPBR->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("Textures\\wood_albedo.png"));
	woodMatPBR->AddPSTextureSRV("NormalTexture", assets.GetTexture("Textures\\wood_normals.png"));
	woodMatPBR->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("Textures\\wood_roughness.png"));
	woodMatPBR->AddPSTextureSRV("MetalTexture", assets.GetTexture("Textures\\wood_metal.png"));
	woodMatPBR->AddPSSampler("BasicSampler", samplerOptions);
	woodMatPBR->AddPSSampler("ClampSampler", clampSampler);

	materials.push_back(cobbleMat2xPBR);
	materials.push_back(floorMatPBR);
	materials.push_back(paintMatPBR);
	materials.push_back(scratchedMatPBR);
	materials.push_back(bronzeMatPBR);
	materials.push_back(roughMatPBR);
	materials.push_back(woodMatPBR);


	// === Create the PBR entities =====================================
	Mesh* sphereMesh = assets.GetMesh("Models\\sphere.obj");
	Mesh* cubeMesh = assets.GetMesh("Models\\cube.obj");

	GameEntity* cobSpherePBR = new GameEntity(sphereMesh, cobbleMat2xPBR);
	cobSpherePBR->GetTransform()->SetScale(2, 2, 2);
	cobSpherePBR->GetTransform()->SetPosition(-6, 2, 0);

	GameEntity* floorSpherePBR = new GameEntity(sphereMesh, floorMatPBR);
	floorSpherePBR->GetTransform()->SetScale(2, 2, 2);
	floorSpherePBR->GetTransform()->SetPosition(-4, 2, 0);

	GameEntity* paintSpherePBR = new GameEntity(sphereMesh, paintMatPBR);
	paintSpherePBR->GetTransform()->SetScale(2, 2, 2);
	paintSpherePBR->GetTransform()->SetPosition(-2, 2, 0);

	GameEntity* scratchSpherePBR = new GameEntity(sphereMesh, scratchedMatPBR);
	scratchSpherePBR->GetTransform()->SetScale(2, 2, 2);
	scratchSpherePBR->GetTransform()->SetPosition(0, 2, 0);

	GameEntity* bronzeSpherePBR = new GameEntity(sphereMesh, bronzeMatPBR);
	bronzeSpherePBR->GetTransform()->SetScale(2, 2, 2);
	bronzeSpherePBR->GetTransform()->SetPosition(2, 2, 0);

	GameEntity* roughSpherePBR = new GameEntity(sphereMesh, roughMatPBR);
	roughSpherePBR->GetTransform()->SetScale(2, 2, 2);
	roughSpherePBR->GetTransform()->SetPosition(4, 2, 0);

	GameEntity* woodSpherePBR = new GameEntity(sphereMesh, woodMatPBR);
	woodSpherePBR->GetTransform()->SetScale(2, 2, 2);
	woodSpherePBR->GetTransform()->SetPosition(6, 2, 0);

	GameEntity* floor = new GameEntity(cubeMesh, woodMatPBR);
	floor->GetTransform()->SetScale(100, 0.1f, 100);
	floor->GetTransform()->SetPosition(0, -5, 0);

	entities.push_back(cobSpherePBR);
	entities.push_back(floorSpherePBR);
	entities.push_back(paintSpherePBR);
	entities.push_back(scratchSpherePBR);
	entities.push_back(bronzeSpherePBR);
	entities.push_back(roughSpherePBR);
	entities.push_back(woodSpherePBR);
	entities.push_back(floor);

	// Create the non-PBR entities ==============================
	GameEntity* cobSphere = new GameEntity(sphereMesh, cobbleMat2x);
	cobSphere->GetTransform()->SetScale(2, 2, 2);
	cobSphere->GetTransform()->SetPosition(-6, -2, 0);

	GameEntity* floorSphere = new GameEntity(sphereMesh, floorMat);
	floorSphere->GetTransform()->SetScale(2, 2, 2);
	floorSphere->GetTransform()->SetPosition(-4, -2, 0);

	GameEntity* paintSphere = new GameEntity(sphereMesh, paintMat);
	paintSphere->GetTransform()->SetScale(2, 2, 2);
	paintSphere->GetTransform()->SetPosition(-2, -2, 0);

	GameEntity* scratchSphere = new GameEntity(sphereMesh, scratchedMat);
	scratchSphere->GetTransform()->SetScale(2, 2, 2);
	scratchSphere->GetTransform()->SetPosition(0, -2, 0);

	GameEntity* bronzeSphere = new GameEntity(sphereMesh, bronzeMat);
	bronzeSphere->GetTransform()->SetScale(2, 2, 2);
	bronzeSphere->GetTransform()->SetPosition(2, -2, 0);

	GameEntity* roughSphere = new GameEntity(sphereMesh, roughMat);
	roughSphere->GetTransform()->SetScale(2, 2, 2);
	roughSphere->GetTransform()->SetPosition(4, -2, 0);

	GameEntity* woodSphere = new GameEntity(sphereMesh, woodMat);
	woodSphere->GetTransform()->SetScale(2, 2, 2);
	woodSphere->GetTransform()->SetPosition(6, -2, 0);

	entities.push_back(cobSphere);
	entities.push_back(floorSphere);
	entities.push_back(paintSphere);
	entities.push_back(scratchSphere);
	entities.push_back(bronzeSphere);
	entities.push_back(roughSphere);
	entities.push_back(woodSphere);


	// Create simple PBR materials & entities (mostly for IBL testing)
	assets.CreateSolidColorTexture("white", 2, 2, XMFLOAT4(1, 1, 1, 1));
	assets.CreateSolidColorTexture("black", 2, 2, XMFLOAT4(0, 0, 0, 0));
	assets.CreateSolidColorTexture("grey", 2, 2, XMFLOAT4(0.5f, 0.5f, 0.5f, 1));
	assets.CreateSolidColorTexture("darkGrey", 2, 2, XMFLOAT4(0.25f, 0.25f, 0.25f, 1));
	assets.CreateSolidColorTexture("flatNormalMap", 2, 2, XMFLOAT4(0.5f, 0.5f, 1.0f, 1.0f));

	Material* solidShinyMetal = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 0.0f, XMFLOAT2(1, 1));
	solidShinyMetal->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("white"));
	solidShinyMetal->AddPSTextureSRV("NormalTexture", assets.GetTexture("flatNormalMap"));
	solidShinyMetal->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("black"));
	solidShinyMetal->AddPSTextureSRV("MetalTexture", assets.GetTexture("white"));
	solidShinyMetal->AddPSSampler("BasicSampler", samplerOptions);
	solidShinyMetal->AddPSSampler("ClampSampler", clampSampler);
	materials.push_back(solidShinyMetal);

	Material* solidQuarterRoughMetal = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 0.0f, XMFLOAT2(1, 1));
	solidQuarterRoughMetal->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("white"));
	solidQuarterRoughMetal->AddPSTextureSRV("NormalTexture", assets.GetTexture("flatNormalMap"));
	solidQuarterRoughMetal->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("darkGrey"));
	solidQuarterRoughMetal->AddPSTextureSRV("MetalTexture", assets.GetTexture("white"));
	solidQuarterRoughMetal->AddPSSampler("BasicSampler", samplerOptions);
	solidQuarterRoughMetal->AddPSSampler("ClampSampler", clampSampler);
	materials.push_back(solidQuarterRoughMetal);

	Material* solidHalfRoughMetal = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 0.0f, XMFLOAT2(1, 1));
	solidHalfRoughMetal->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("white"));
	solidHalfRoughMetal->AddPSTextureSRV("NormalTexture", assets.GetTexture("flatNormalMap"));
	solidHalfRoughMetal->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("grey"));
	solidHalfRoughMetal->AddPSTextureSRV("MetalTexture", assets.GetTexture("white"));
	solidHalfRoughMetal->AddPSSampler("BasicSampler", samplerOptions);
	solidHalfRoughMetal->AddPSSampler("ClampSampler", clampSampler);
	materials.push_back(solidHalfRoughMetal);

	Material* solidShinyPlastic = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 0.0f, XMFLOAT2(1, 1));
	solidShinyPlastic->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("white"));
	solidShinyPlastic->AddPSTextureSRV("NormalTexture", assets.GetTexture("flatNormalMap"));
	solidShinyPlastic->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("black"));
	solidShinyPlastic->AddPSTextureSRV("MetalTexture", assets.GetTexture("black"));
	solidShinyPlastic->AddPSSampler("BasicSampler", samplerOptions);
	solidShinyPlastic->AddPSSampler("ClampSampler", clampSampler);
	materials.push_back(solidShinyPlastic);

	Material* solidQuarterRoughPlastic = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 0.0f, XMFLOAT2(1, 1));
	solidQuarterRoughPlastic->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("white"));
	solidQuarterRoughPlastic->AddPSTextureSRV("NormalTexture", assets.GetTexture("flatNormalMap"));
	solidQuarterRoughPlastic->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("darkGrey"));
	solidQuarterRoughPlastic->AddPSTextureSRV("MetalTexture", assets.GetTexture("black"));
	solidQuarterRoughPlastic->AddPSSampler("BasicSampler", samplerOptions);
	solidQuarterRoughPlastic->AddPSSampler("ClampSampler", clampSampler);
	materials.push_back(solidQuarterRoughPlastic);

	Material* solidHalfRoughPlastic = new Material(vs, psPBR, XMFLOAT4(1, 1, 1, 1), 0.0f, XMFLOAT2(1, 1));
	solidHalfRoughPlastic->AddPSTextureSRV("AlbedoTexture", assets.GetTexture("white"));
	solidHalfRoughPlastic->AddPSTextureSRV("NormalTexture", assets.GetTexture("flatNormalMap"));
	solidHalfRoughPlastic->AddPSTextureSRV("RoughnessTexture", assets.GetTexture("grey"));
	solidHalfRoughPlastic->AddPSTextureSRV("MetalTexture", assets.GetTexture("black"));
	solidHalfRoughPlastic->AddPSSampler("BasicSampler", samplerOptions);
	solidHalfRoughPlastic->AddPSSampler("ClampSampler", clampSampler);
	materials.push_back(solidHalfRoughPlastic);



	GameEntity* shinyMetal = new GameEntity(sphereMesh, solidShinyMetal);
	shinyMetal->GetTransform()->SetPosition(-5, 0, 0);
	entities.push_back(shinyMetal);

	GameEntity* quarterRoughMetal = new GameEntity(sphereMesh, solidQuarterRoughMetal);
	quarterRoughMetal->GetTransform()->SetPosition(-3.5f, 0, 0);
	entities.push_back(quarterRoughMetal);

	GameEntity* roughMetal = new GameEntity(sphereMesh, solidHalfRoughMetal);
	roughMetal->GetTransform()->SetPosition(-2, 0, 0);
	entities.push_back(roughMetal);

	GameEntity* shinyPlastic = new GameEntity(sphereMesh, solidShinyPlastic);
	shinyPlastic->GetTransform()->SetPosition(2, 0, 0);
	entities.push_back(shinyPlastic);

	GameEntity* quarterRoughPlastic = new GameEntity(sphereMesh, solidQuarterRoughPlastic);
	quarterRoughPlastic->GetTransform()->SetPosition(3.5f, 0, 0);
	entities.push_back(quarterRoughPlastic);

	GameEntity* roughPlastic = new GameEntity(sphereMesh, solidHalfRoughPlastic);
	roughPlastic->GetTransform()->SetPosition(5, 0, 0);
	entities.push_back(roughPlastic);


	// Transform test =====================================
	entities[0]->GetTransform()->AddChild(entities[1]->GetTransform(), true);
}


// --------------------------------------------------------
// Generates the lights in the scene: 3 directional lights
// and many random point lights.
// --------------------------------------------------------
void Game::GenerateLights()
{
	// Reset
	lights.clear();

	// Setup directional lights
	Light dir1 = {};
	dir1.Type = LIGHT_TYPE_DIRECTIONAL;
	dir1.Direction = XMFLOAT3(1, -1, 1);
	dir1.Color = XMFLOAT3(0.8f, 0.8f, 0.8f);
	dir1.Intensity = 1.0f;

	Light dir2 = {};
	dir2.Type = LIGHT_TYPE_DIRECTIONAL;
	dir2.Direction = XMFLOAT3(-1, -0.25f, 0);
	dir2.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir2.Intensity = 1.0f;

	Light dir3 = {};
	dir3.Type = LIGHT_TYPE_DIRECTIONAL;
	dir3.Direction = XMFLOAT3(0, -1, 1);
	dir3.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir3.Intensity = 1.0f;

	// Add light to the list
	lights.push_back(dir1);
	lights.push_back(dir2);
	lights.push_back(dir3);

	// Create the rest of the lights
	while (lights.size() < lightCount)
	{
		Light point = {};
		point.Type = LIGHT_TYPE_POINT;
		point.Position = XMFLOAT3(RandomRange(-10.0f, 10.0f), RandomRange(-5.0f, 5.0f), RandomRange(-10.0f, 10.0f));
		point.Color = XMFLOAT3(RandomRange(0, 1), RandomRange(0, 1), RandomRange(0, 1));
		point.Range = RandomRange(5.0f, 10.0f);
		point.Intensity = RandomRange(0.1f, 3.0f);

		// Add to the list
		lights.push_back(point);
	}

}



// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	renderer->PostResize(this->width, this->height, backBufferRTV, depthStencilView);

	// Update our projection matrix to match the new aspect ratio
	if (camera)
		camera->UpdateProjectionMatrix(this->width / (float)this->height);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{

	// Reset input manager's gui state so we don’t
	// taint our own input (you’ll uncomment later)
	input.SetGuiKeyboardCapture(false);
	input.SetGuiMouseCapture(false);
	// Set io info
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->width;
	io.DisplaySize.y = (float)this->height;
	io.KeyCtrl = input.KeyDown(VK_CONTROL);
	io.KeyShift = input.KeyDown(VK_SHIFT);
	io.KeyAlt = input.KeyDown(VK_MENU);
	io.MousePos.x = (float)input.GetMouseX();
	io.MousePos.y = (float)input.GetMouseY();
	io.MouseDown[0] = input.MouseLeftDown();
	io.MouseDown[1] = input.MouseRightDown();
	io.MouseDown[2] = input.MouseMiddleDown();
	io.MouseWheel = input.GetMouseWheel();
	input.GetKeyArray(io.KeysDown, 256);

	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Determine new input capture (you’ll uncomment later)
	input.SetGuiKeyboardCapture(io.WantCaptureKeyboard);
	input.SetGuiMouseCapture(io.WantCaptureMouse);

	// Show the demo window
	//ImGui:: ShowDemoWindow();

	ImGui::Begin("Stats");

	ImGui::Text("Framerate: ");
	ImGui::SameLine();
	ImGui::Text(std::to_string(io.Framerate).c_str());
	ImGui::Text("Width: ");
	ImGui::SameLine();
	ImGui::Text(std::to_string(width).c_str());
	ImGui::SameLine();
	ImGui::Text("Height: ");
	ImGui::SameLine();
	ImGui::Text(std::to_string(height).c_str());
	ImGui::Text("Aspect Ratio: ");
	ImGui::SameLine();
	ImGui::Text(std::to_string((float)width / (float)height).c_str());
	ImGui::Text("Entity Count");
	ImGui::SameLine();
	ImGui::Text(std::to_string(entities.size()).c_str());
	ImGui::Text("\nPlayer Position");
	ImGui::Text("X: ");
	ImGui::SameLine();
	ImGui::Text(std::to_string(camera->GetTransform()->GetPosition().x).c_str());
	ImGui::SameLine();
	ImGui::Text(" Y: ");
	ImGui::SameLine();
	ImGui::Text(std::to_string(camera->GetTransform()->GetPosition().y).c_str());
	ImGui::SameLine();
	ImGui::Text(" Z: ");
	ImGui::SameLine();
	ImGui::Text(std::to_string(camera->GetTransform()->GetPosition().z).c_str());


	ImGui::End();

	ImGui::Begin("Object Manager");
	if (ImGui::CollapsingHeader("Lights"))
	{
		for (size_t i = 0; i < lights.size(); i++)
		{
			if (ImGui::TreeNode(("Light " + std::to_string(i + 1)).c_str()))
			{
				ImGui::DragFloat3("Position", &lights[i].Position.x);
				ImGui::DragFloat("Intensity", &lights[i].Intensity);
				ImGui::ColorEdit4("Color", &lights[i].Color.x);
				ImGui::TreePop();
			}
		}
	}
	
	if (ImGui::CollapsingHeader("Entities"))
	{
		for (size_t i = 0; i < entities.size(); i++)
		{
			if (ImGui::TreeNode(("Object" + std::to_string(i + 1)).c_str()))
			{
				XMFLOAT3 pos = entities[i]->GetTransform()->GetPosition();
				XMFLOAT3 rot = entities[i]->GetTransform()->GetPitchYawRoll();
				XMFLOAT3 scl = entities[i]->GetTransform()->GetScale();
				ImGui::DragFloat3("Position", &pos.x, 0.5f);
				ImGui::DragFloat3("Rotation", &rot.x, 0.1f);
				ImGui::DragFloat3("Scale", &scl.x, 0.5f);
				entities[i]->GetTransform()->SetPosition(pos.x, pos.y, pos.z);
				entities[i]->GetTransform()->SetRotation(rot.x, rot.y, rot.z);
				entities[i]->GetTransform()->SetScale(scl.x, scl.y, scl.z);
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();

	ImGui::Begin("Network Manager");

	NetworkState s = netManager->GetNetworkState();
	if (s == NetworkState::Offline)
	{
		ImGui::InputText("##text1", ip, 15);
		ImGui::InputText("##text2", port, 6);

		if (ImGui::Button("Connect"))
		{
			char* pEnd;
			netManager->Connect(ip, strtol(port, &pEnd, 0));
		}
	}
	else if (s == NetworkState::Connecting)
	{
		ImGui::Text("Connecting...");
	}
	else if (s == NetworkState::Connected)
	{
		ImGui::Text("Connected!");

		if (ImGui::Button("Disconnect"))
		{
			netManager->Disconnect();
		}
	}
	ImGui::End();

	Input& input = Input::GetInstance();

	// Update the camera
	camera->Update(deltaTime);
	localPlayer->Update(deltaTime);

	if (input.MouseLeftPress())
	{
		Projectile* bullet = new Projectile(Assets::GetInstance().GetMesh("Models\\sphere.obj"), materials[0], 5);
		projectiles.push_back(bullet);
		entities.push_back(bullet);
		Transform* tf = bullet->GetTransform();
		tf->SetScale(0.2f, 0.2f, 0.2f);
		Transform* camtf = localPlayer->GetCamera()->GetTransform();
		tf->SetPosition(camtf->GetPosition().x + localPlayer->velocityX * deltaTime, camtf->GetPosition().y + localPlayer->velocityY * deltaTime, camtf->GetPosition().z + localPlayer->velocityZ * deltaTime);
		tf->SetRotation(camtf->GetPitchYawRoll().x, camtf->GetPitchYawRoll().y, camtf->GetPitchYawRoll().z);
		bullet->SetVelocity(0, 5, 20, -9.8f);
	}

	for (int i = 0; i < projectiles.size(); i++)
	{
		Projectile* p = projectiles[i];
		p->Update(deltaTime);
		if (p->dead)
		{
			auto it = find(entities.begin(), entities.end(), p);
			auto it2 = find(projectiles.begin(), projectiles.end(), p);
			entities.erase(it);
			projectiles.erase(it2);
			delete p;
		}
	}

	netManager->Update(deltaTime);

	// Check individual input
	if (input.KeyDown(VK_ESCAPE)) Quit();
	if (input.KeyPress(VK_TAB)) GenerateLights();

}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	renderer->Render(camera, lightCount, lightVS, lightPS, lightMesh);
}


// --------------------------------------------------------
// Draws a simple informational "UI" using sprite batch
// --------------------------------------------------------
/*void Game::DrawUI()
{
	spriteBatch->Begin();

	// Basic controls
	float h = 10.0f;
	arial->DrawString(spriteBatch, L"Controls:", XMVectorSet(10, h, 0, 0));
	arial->DrawString(spriteBatch, L" (WASD, X, Space) Move camera", XMVectorSet(10, h + 20, 0, 0));
	arial->DrawString(spriteBatch, L" (Left Click & Drag) Rotate camera", XMVectorSet(10, h + 40, 0, 0));
	arial->DrawString(spriteBatch, L" (Left Shift) Hold to speed up camera", XMVectorSet(10, h + 60, 0, 0));
	arial->DrawString(spriteBatch, L" (Left Ctrl) Hold to slow down camera", XMVectorSet(10, h + 80, 0, 0));
	arial->DrawString(spriteBatch, L" (TAB) Randomize lights", XMVectorSet(10, h + 100, 0, 0));

	// Current "scene" info
	h = 150;
	arial->DrawString(spriteBatch, L"Scene Details:", XMVectorSet(10, h, 0, 0));
	arial->DrawString(spriteBatch, L" Top: PBR materials", XMVectorSet(10, h + 20, 0, 0));
	arial->DrawString(spriteBatch, L" Bottom: Non-PBR materials", XMVectorSet(10, h + 40, 0, 0));

	spriteBatch->End();

	// Reset render states, since sprite batch changes these!
	context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	context->OMSetDepthStencilState(0, 0);

}*/
