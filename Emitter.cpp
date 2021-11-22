#include "Emitter.h"

Emitter::Emitter(int maxParticles, int particlesPerSecond, float lifetime, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, SimpleVertexShader* vs, SimplePixelShader* ps, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture)
	: maxParticles(maxParticles), particlesPerSecond(particlesPerSecond), particleLifeSpan(lifetime), context(context), vs(vs), ps(ps), texture(texture)
{

	secondsPerParticle = 1.0f / particlesPerSecond;

	dtSinceLastEmit = 0.0f;
	livingCount = 0;
	livingStart = 0;
	deadStart = 0;

	particles = new Particle[maxParticles];
	ZeroMemory(particles, sizeof(Particle) * maxParticles);

	unsigned int* indices = new unsigned int[maxParticles * 6];
	int indexCount = 0;
	for (int i = 0; i < maxParticles * 4; i += 4)
	{
		indices[indexCount++] = i;
		indices[indexCount++] = i + 1;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i + 3;
	}
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices;

	D3D11_BUFFER_DESC buffDesc = {};
	buffDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	buffDesc.CPUAccessFlags = 0;
	buffDesc.Usage = D3D11_USAGE_DEFAULT;
	buffDesc.ByteWidth = sizeof(unsigned int) * maxParticles * 6;
	device->CreateBuffer(&buffDesc, &indexData, indexBuffer.GetAddressOf());
	delete[] indices;

	D3D11_BUFFER_DESC particlesBufferDesc = {};
	particlesBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	particlesBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	particlesBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	particlesBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	particlesBufferDesc.StructureByteStride = sizeof(Particle);
	particlesBufferDesc.ByteWidth = sizeof(Particle) * maxParticles;
	device->CreateBuffer(&particlesBufferDesc, 0, particleDataBuffer.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxParticles;
	device->CreateShaderResourceView(particleDataBuffer.Get(), &srvDesc, particleDataSRV.GetAddressOf());

}

Emitter::~Emitter()
{
	delete[] particles;
}

void Emitter::Update(float dt, float currentTime)
{

	if (livingCount > 0)
	{
		if (livingStart < deadStart)
		{
			for (int i = livingStart; i < deadStart; i++)
			{
				UpdateParticle(currentTime, i);
			}
		}
		else if (livingStart > deadStart)
		{
			for (int i = livingStart; i < maxParticles; i++)
			{
				UpdateParticle(currentTime, i);
			}
			for (int i = 0; i < deadStart; i++)
			{
				UpdateParticle(currentTime, i);
			}
		}
		else
		{
			for (int i = 0; i < maxParticles; i++)
			{
				UpdateParticle(currentTime, i);
			}
		}
	}

	dtSinceLastEmit += dt;

	while (dtSinceLastEmit > secondsPerParticle)
	{
		EmitParticle(currentTime);
		dtSinceLastEmit -= secondsPerParticle;
	}

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	context->Map(particleDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	if (livingStart < deadStart)
	{
		memcpy(mapped.pData, particles + livingStart, sizeof(Particle) * livingCount);
	}
	else
	{
		memcpy(mapped.pData, particles, sizeof(Particle) * deadStart);
		memcpy((void*)((Particle*)mapped.pData + deadStart), particles + livingStart, sizeof(Particle) * (maxParticles - livingStart));
	}

	context->Unmap(particleDataBuffer.Get(), 0);

}

void Emitter::Draw(Camera* camera, float currentTime)
{

	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = 0;
	context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	vs->SetShader();
	ps->SetShader();

	vs->SetShaderResourceView("ParticleData", particleDataSRV);
	ps->SetShaderResourceView("Texture", texture);

	vs->SetMatrix4x4("view", camera->GetView());
	vs->SetMatrix4x4("projection", camera->GetProjection());
	vs->SetFloat("currentTime", currentTime);
	vs->CopyAllBufferData();

	context->DrawIndexed(livingCount * 6, 0, 0);

}

void Emitter::UpdateParticle(float currentTime, int index)
{
	float age = currentTime - particles[index].emitTime;

	if (age >= particleLifeSpan)
	{
		livingStart++;
		livingStart %= maxParticles;
		livingCount--;
	}

}

void Emitter::EmitParticle(float currentTime)
{

	if (livingCount == maxParticles)
		return;

	int newIndex = deadStart;
	particles[newIndex].emitTime = currentTime;
	particles[newIndex].initPos = DirectX::XMFLOAT3(0, 0, 0);

	//fun stuff

	deadStart++;
	deadStart %= maxParticles;

	livingCount++;

}
