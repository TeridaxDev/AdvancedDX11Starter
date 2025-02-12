#pragma once
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include <unordered_map>
#include <vector>
#include <string>


// --------------------------------------------------------
// Used by simple shaders to store information about
// specific variables in constant buffers
// --------------------------------------------------------
struct SimpleShaderVariable
{
	unsigned int ByteOffset;
	unsigned int Size;
	unsigned int ConstantBufferIndex;
};

// --------------------------------------------------------
// Contains information about a specific
// constant buffer in a shader, as well as
// the local data buffer for it
// --------------------------------------------------------
struct SimpleConstantBuffer
{
	std::string Name;
	D3D_CBUFFER_TYPE Type = D3D_CBUFFER_TYPE::D3D11_CT_CBUFFER;
	unsigned int Size = 0;
	unsigned int BindIndex = 0;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ConstantBuffer = 0;
	unsigned char* LocalDataBuffer = 0;
	std::vector<SimpleShaderVariable> Variables;
};

// --------------------------------------------------------
// Contains info about a single SRV in a shader
// --------------------------------------------------------
struct SimpleSRV
{
	unsigned int Index;		// The raw index of the SRV
	unsigned int BindIndex; // The register of the SRV
};

// --------------------------------------------------------
// Contains info about a single Sampler in a shader
// --------------------------------------------------------
struct SimpleSampler
{
	unsigned int Index;		// The raw index of the Sampler
	unsigned int BindIndex; // The register of the Sampler
};

// --------------------------------------------------------
// Base abstract class for simplifying shader handling
// --------------------------------------------------------
class ISimpleShader
{
public:
	ISimpleShader(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	virtual ~ISimpleShader();

	// Simple helpers
	bool IsShaderValid() { return shaderValid; }

	// Activating the shader and copying data
	void SetShader();
	void CopyAllBufferData();
	void CopyBufferData(unsigned int index);
	void CopyBufferData(std::string bufferName);

	// Sets arbitrary shader data
	bool SetData(std::string name, const void* data, unsigned int size);

	bool SetInt(std::string name, int data);
	bool SetFloat(std::string name, float data);
	bool SetFloat2(std::string name, const float data[2]);
	bool SetFloat2(std::string name, const DirectX::XMFLOAT2 data);
	bool SetFloat3(std::string name, const float data[3]);
	bool SetFloat3(std::string name, const DirectX::XMFLOAT3 data);
	bool SetFloat4(std::string name, const float data[4]);
	bool SetFloat4(std::string name, const DirectX::XMFLOAT4 data);
	bool SetMatrix4x4(std::string name, const float data[16]);
	bool SetMatrix4x4(std::string name, const DirectX::XMFLOAT4X4 data);

	// Setting shader resources
	virtual bool SetShaderResourceView(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) = 0;
	virtual bool SetSamplerState(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState) = 0;

	// Simple resource checking
	bool HasVariable(std::string name);
	bool HasShaderResourceView(std::string name);
	bool HasSamplerState(std::string name);

	// Getting data about variables and resources
	const SimpleShaderVariable* GetVariableInfo(std::string name);

	const SimpleSRV* GetShaderResourceViewInfo(std::string name);
	const SimpleSRV* GetShaderResourceViewInfo(unsigned int index);
	size_t GetShaderResourceViewCount() { return textureTable.size(); }

	const SimpleSampler* GetSamplerInfo(std::string name);
	const SimpleSampler* GetSamplerInfo(unsigned int index);
	size_t GetSamplerCount() { return samplerTable.size(); }

	// Get data about constant buffers
	unsigned int GetBufferCount();
	unsigned int GetBufferSize(unsigned int index);
	const SimpleConstantBuffer* GetBufferInfo(std::string name);
	const SimpleConstantBuffer* GetBufferInfo(unsigned int index);

	// Misc getters
	Microsoft::WRL::ComPtr<ID3DBlob> GetShaderBlob() { return shaderBlob; }

	// Error reporting
	static bool ReportErrors;
	static bool ReportWarnings;

protected:

	bool shaderValid;
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;

	// Resource counts
	unsigned int constantBufferCount;

	// Maps for variables and buffers
	SimpleConstantBuffer* constantBuffers; // For index-based lookup
	std::vector<SimpleSRV*>		shaderResourceViews;
	std::vector<SimpleSampler*>	samplerStates;
	std::unordered_map<std::string, SimpleConstantBuffer*> cbTable;
	std::unordered_map<std::string, SimpleShaderVariable> varTable;
	std::unordered_map<std::string, SimpleSRV*> textureTable;
	std::unordered_map<std::string, SimpleSampler*> samplerTable;

	// Initialization method
	bool LoadShaderFile(LPCWSTR shaderFile);

	// Pure virtual functions for dealing with shader types
	virtual bool CreateShader(Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob) = 0;
	virtual void SetShaderAndCBs() = 0;

	virtual void CleanUp();

	// Helpers for finding data by name
	SimpleShaderVariable* FindVariable(std::string name, int size);
	SimpleConstantBuffer* FindConstantBuffer(std::string name);

	// Error logging
	void Log(std::string message, WORD color);
	void LogW(std::wstring message, WORD color);
	void Log(std::string message);
	void LogW(std::wstring message);
	void LogError(std::string message);
	void LogErrorW(std::wstring message);
	void LogWarning(std::string message);
	void LogWarningW(std::wstring message);
};

// --------------------------------------------------------
// Derived class for VERTEX shaders ///////////////////////
// --------------------------------------------------------
class SimpleVertexShader : public ISimpleShader
{
public:
	SimpleVertexShader(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, LPCWSTR shaderFile);
	SimpleVertexShader(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, LPCWSTR shaderFile, Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout, bool perInstanceCompatible);
	~SimpleVertexShader();
	Microsoft::WRL::ComPtr<ID3D11VertexShader> GetDirectXShader() { return shader; }
	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout() { return inputLayout; }
	bool GetPerInstanceCompatible() { return perInstanceCompatible; }

	bool SetShaderResourceView(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	bool SetSamplerState(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState);

protected:
	bool perInstanceCompatible;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> shader;
	bool CreateShader(Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();
};


// --------------------------------------------------------
// Derived class for PIXEL shaders ////////////////////////
// --------------------------------------------------------
class SimplePixelShader : public ISimpleShader
{
public:
	SimplePixelShader(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, LPCWSTR shaderFile);
	~SimplePixelShader();
	Microsoft::WRL::ComPtr<ID3D11PixelShader> GetDirectXShader() { return shader; }

	bool SetShaderResourceView(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	bool SetSamplerState(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState);

protected:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> shader;
	bool CreateShader(Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();
};

// --------------------------------------------------------
// Derived class for DOMAIN shaders ///////////////////////
// --------------------------------------------------------
class SimpleDomainShader : public ISimpleShader
{
public:
	SimpleDomainShader(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, LPCWSTR shaderFile);
	~SimpleDomainShader();
	Microsoft::WRL::ComPtr<ID3D11DomainShader> GetDirectXShader() { return shader; }

	bool SetShaderResourceView(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	bool SetSamplerState(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState);

protected:
	Microsoft::WRL::ComPtr<ID3D11DomainShader> shader;
	bool CreateShader(Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();
};

// --------------------------------------------------------
// Derived class for HULL shaders /////////////////////////
// --------------------------------------------------------
class SimpleHullShader : public ISimpleShader
{
public:
	SimpleHullShader(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, LPCWSTR shaderFile);
	~SimpleHullShader();
	Microsoft::WRL::ComPtr<ID3D11HullShader> GetDirectXShader() { return shader; }

	bool SetShaderResourceView(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	bool SetSamplerState(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState);

protected:
	Microsoft::WRL::ComPtr<ID3D11HullShader> shader;
	bool CreateShader(Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();
};

// --------------------------------------------------------
// Derived class for GEOMETRY shaders /////////////////////
// --------------------------------------------------------
class SimpleGeometryShader : public ISimpleShader
{
public:
	SimpleGeometryShader(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, LPCWSTR shaderFile, bool useStreamOut = 0, bool allowStreamOutRasterization = 0);
	~SimpleGeometryShader();
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> GetDirectXShader() { return shader; }

	bool SetShaderResourceView(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	bool SetSamplerState(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState);

	bool CreateCompatibleStreamOutBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> buffer, int vertexCount);

	static void UnbindStreamOutStage(Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext);

protected:
	// Shader itself
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> shader;

	// Stream out related
	bool useStreamOut;
	bool allowStreamOutRasterization;
	unsigned int streamOutVertexSize;

	bool CreateShader(Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob);
	bool CreateShaderWithStreamOut(Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();

	// Helpers
	unsigned int CalcComponentCount(unsigned int mask);
};


// --------------------------------------------------------
// Derived class for COMPUTE shaders //////////////////////
// --------------------------------------------------------
class SimpleComputeShader : public ISimpleShader
{
public:
	SimpleComputeShader(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, LPCWSTR shaderFile);
	~SimpleComputeShader();
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> GetDirectXShader() { return shader; }

	void DispatchByGroups(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ);
	void DispatchByThreads(unsigned int threadsX, unsigned int threadsY, unsigned int threadsZ);

	bool HasUnorderedAccessView(std::string name);

	bool SetShaderResourceView(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	bool SetSamplerState(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState);
	bool SetUnorderedAccessView(std::string name, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav, unsigned int appendConsumeOffset = -1);

	int GetUnorderedAccessViewIndex(std::string name);

protected:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader;
	std::unordered_map<std::string, unsigned int> uavTable;

	unsigned int threadsX;
	unsigned int threadsY;
	unsigned int threadsZ;
	unsigned int threadsTotal;

	bool CreateShader(Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();
};