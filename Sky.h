#pragma once

#include "Mesh.h"
#include "SimpleShader.h"
#include "Camera.h"

#include <wrl/client.h> // Used for ComPtr

class Sky
{
public:

	// Constructor that loads a DDS cube map file
	Sky(
		const wchar_t* cubemapDDSFile, 
		Mesh* mesh, 
		SimpleVertexShader* skyVS,
		SimplePixelShader* skyPS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 	
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context
	);

	// Constructor that takes 6 existing SRVs and makes a cube map
	Sky(
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> right,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> left,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> up,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> down,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> front,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> back,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context
	);

	~Sky();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> IBLGetIrradianceMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> IBLGetConvolvedSpecularMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> IBLGetBRDFLookupTexture();
	int IBLGetMipLevels();

	void Draw(Camera* camera);

private:

	void InitRenderStates();

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> right,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> left,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> up,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> down,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> front,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> back);

	// Skybox related resources
	SimpleVertexShader* skyVS;
	SimplePixelShader* skyPS;
	
	Mesh* skyMesh;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyRasterState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyDepthState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skySRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceCM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> convolvedSpecularCM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> lookupTexture;
	int mipLevels;
	const int mipSkip = 3;
	const int mipFaceSize = 512;
	const int lookupSize = 512;

	void IBLCreateIrradianceMap();
	void IBLCreateConvolvedSpecularMap();
	void IBLCreateBRDFLookUpTexture();

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
};

