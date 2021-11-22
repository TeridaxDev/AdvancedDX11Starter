#pragma once

#include <wrl/client.h>
#include <DirectXMath.h>
#include <d3d11.h>

#include "SimpleShader.h"
#include "Camera.h"

struct Particle
{
	float emitTime;
	DirectX::XMFLOAT3 initPos; //16 Bytes
};

class Emitter
{
public:


	Emitter(
		int maxParticles,
		int particlesPerSecond,
		float lifetime,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		SimpleVertexShader* vs,
		SimplePixelShader* ps,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture
	);
	~Emitter();

	void Update(float dt, float currentTime);
	void Draw(Camera* camera, float currentTime);


private:
	Particle* particles;
	int maxParticles;
	int livingStart;
	int deadStart;
	int livingCount;

	//Emission
	int particlesPerSecond;
	float secondsPerParticle;
	float dtSinceLastEmit;

	//System
	float particleLifeSpan;

	//GPU
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11Buffer> particleDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	SimpleVertexShader* vs;
	SimplePixelShader* ps;

	//Methods
	void UpdateParticle(float currentTime, int index);
	void EmitParticle(float currentTime);


};

