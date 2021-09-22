#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include "GameEntity.h"
#include "Sky.h"
#include "Lights.h"

class Renderer
{

private:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;

	unsigned int windowWidth;
	unsigned int windowHeight;

	Sky* sky;

	const std::vector<GameEntity*>& entities;
	const std::vector<Light>& lights;

public:

	//Constructor start
	Renderer(Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
		unsigned int windowWidth,
		unsigned int windowHeight,
		Sky* sky,
		std::vector<GameEntity*>& entities,
		std::vector<Light>& lights) 
		: device(device), context(context), swapChain(swapChain), backBufferRTV(backBufferRTV), depthBufferDSV(depthBufferDSV),
		windowWidth(windowWidth), windowHeight(windowHeight), sky(sky),
		entities(entities), lights(lights) {}
	//Constructor end



	void PostResize(unsigned int windowWidth, unsigned int windowHeight, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV);

	void Render(Camera* camera, int lightCount, SimpleVertexShader* lightVS, SimplePixelShader* lightPS, Mesh* lightMesh);

	void DrawPointLights(Camera* camera, int lightCount, SimpleVertexShader* lightVS, SimplePixelShader* lightPS, Mesh* lightMesh);

};

