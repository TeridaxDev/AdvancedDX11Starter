#include "AssetLoader.h"
#include <unordered_map>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#include <iostream>
#include <d3d11.h>
#include "WICTextureLoader.h"
#include <string>
#include "DXCore.h"

#define LoadTexture(file, srv) CreateWICTextureFromFile(device.Get(), context.Get(), GetFullPathTo_Wide(file).c_str(), 0, srv.GetAddressOf())
#define LoadShader(type, file) new type(device.Get(), context.Get(), GetFullPathTo_Wide(file).c_str())

namespace fs = std::experimental::filesystem;

void AssetLoader::LoadAssets(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	
	for (auto& p : fs::recursive_directory_iterator("Assets"))
	{

		if (p.path().extension() == ".obj")
		{
			std::string string = GetFullPathTo(p.path().filename().u8string());
			std::shared_ptr<Mesh> mesh(new Mesh(string.c_str(), device));
			
			std::pair<const char*, std::shared_ptr<Mesh>> pair(p.path().filename().u8string().c_str(), mesh);
			meshes.insert(pair);
		}
		else if (p.path().extension() == ".png" || p.path().extension() == ".jpg")
		{
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texpoin;
			std::wstring string = GetFullPathTo_Wide(p.path().wstring());
			DirectX::CreateWICTextureFromFile(device.Get(), context.Get(), string.c_str(), 0, texpoin.GetAddressOf());
			std::pair<const char*, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> pair(p.path().filename().u8string().c_str(), texpoin);
			textures.insert(pair);
		}
		else if (p.path().extension() == "") {}
		else
		{
			printf("Unhandled type ");
			#if defined(DEBUG) || defined(_DEBUG)
				std::cout << p.path().extension() << std::endl;
			#endif	
		}

	}


}

void AssetLoader::DeleteAssets()
{
}

Mesh* AssetLoader::GetMesh(const char* addr)
{
	return meshes[addr].get();
}

ID3D11ShaderResourceView* AssetLoader::GetTexture(const char* addr)
{
	return textures[addr].Get();
}
