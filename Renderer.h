#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include "GameEntity.h"
#include "Sky.h"
#include "Lights.h"
#include "Emitter.h"

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
	const std::vector<Emitter*>& emitters;

	// SSAO variables
	DirectX::XMFLOAT4 ssaoOffsets[64];
	int ssaoSamples;
	float ssaoRadius;

	DirectX::XMFLOAT3 ambientNonPBR;

	//Alt Render Targets
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneColorsRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneNormalsRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneAmbientRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneDepthRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ssaoResultRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ssaoBlurRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneColorsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneNormalsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneAmbientSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneDepthSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ssaoResultSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ssaoBlurSRV;
	Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> particleDepthState;

	void CreateGenericRenderTarget(unsigned int width, unsigned int height, Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM);


public:

	Renderer(Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
		unsigned int windowWidth,
		unsigned int windowHeight,
		Sky* sky,
		std::vector<GameEntity*>& entities,
		std::vector<Light>& lights,
		std::vector<Emitter*>& emitters);



	void PostResize(unsigned int windowWidth, unsigned int windowHeight, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV);

	void Render(Camera* camera, float totalTime, int lightCount, SimpleVertexShader* lightVS, SimplePixelShader* lightPS, Mesh* lightMesh);

	void DrawPointLights(Camera* camera, int lightCount, SimpleVertexShader* lightVS, SimplePixelShader* lightPS, Mesh* lightMesh);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSceneColorsSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSceneNormalsSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSceneAmbientSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSceneDepthSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSSAO();

};

