#pragma once

#include <unordered_map>
#include <wrl/client.h>
#include "Mesh.h"
#include <memory>

class AssetLoader
{
public:
    static AssetLoader& getInstance()
    {
        static AssetLoader    instance; 

        return instance;
    }
private:

    std::unordered_map<const char*, std::shared_ptr<Mesh>> meshes;
    std::unordered_map<const char*, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textures;

    AssetLoader() {}

public:
    AssetLoader(AssetLoader const&) = delete;
    void operator=(AssetLoader const&) = delete;

    void LoadAssets(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
    void DeleteAssets();

    Mesh* GetMesh(const char* addr);
    ID3D11ShaderResourceView* GetTexture(const char* addr);

protected:
	// --------------------------------------------------------------------------
// Gets the actual path to this executable
//
// - As it turns out, the relative path for a program is different when 
//    running through VS and when running the .exe directly, which makes 
//    it a pain to properly load files external files (like textures)
//    - Running through VS: Current Dir is the *project folder*
//    - Running from .exe:  Current Dir is the .exe's folder
// - This has nothing to do with DEBUG and RELEASE modes - it's purely a 
//    Visual Studio "thing", and isn't obvious unless you know to look 
//    for it.  In fact, it could be fixed by changing a setting in VS, but
//    the option is stored in a user file (.suo), which is ignored by most
//    version control packages by default.  Meaning: the option must be
//    changed every on every PC.  Ugh.  So instead, here's a helper.
// --------------------------------------------------------------------------
	std::string GetExePath()
	{
		// Assume the path is just the "current directory" for now
		std::string path = ".\\";

		// Get the real, full path to this executable
		char currentDir[1024] = {};
		GetModuleFileName(0, currentDir, 1024);

		// Find the location of the last slash charaacter
		char* lastSlash = strrchr(currentDir, '\\');
		if (lastSlash)
		{
			// End the string at the last slash character, essentially
			// chopping off the exe's file name.  Remember, c-strings
			// are null-terminated, so putting a "zero" character in 
			// there simply denotes the end of the string.
			*lastSlash = 0;

			// Set the remainder as the path
			path = currentDir;
		}

		// Toss back whatever we've found
		return path;
	}


	// ---------------------------------------------------
	//  Same as GetExePath(), except it returns a wide character
	//  string, which most of the Windows API requires.
	// ---------------------------------------------------
	std::wstring GetExePath_Wide()
	{
		// Grab the path as a standard string
		std::string path = GetExePath();

		// Convert to a wide string
		wchar_t widePath[1024] = {};
		mbstowcs_s(0, widePath, path.c_str(), 1024);

		// Create a wstring for it and return
		return std::wstring(widePath);
	}


	// ----------------------------------------------------
	//  Gets the full path to a given file.  NOTE: This does 
	//  NOT "find" the file, it simply concatenates the given
	//  relative file path onto the executable's path
	// ----------------------------------------------------
	std::string GetFullPathTo(std::string relativeFilePath)
	{
		return GetExePath() + "\\" + relativeFilePath;
	}

	// ----------------------------------------------------
//  Same as GetFullPathTo, but with wide char strings.
// 
//  Gets the full path to a given file.  NOTE: This does 
//  NOT "find" the file, it simply concatenates the given
//  relative file path onto the executable's path
// ----------------------------------------------------
	std::wstring GetFullPathTo_Wide(std::wstring relativeFilePath)
	{
		return GetExePath_Wide() + L"\\" + relativeFilePath;
	}

};