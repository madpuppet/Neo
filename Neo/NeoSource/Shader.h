#pragma once

/*
	Shader -> actual resource ready to use for rendering
	ShaderRef -> reference a resource and use it to create/destroy it in a refcounted way
	ShaderFactory -> keeps a cache of all created textures resources
	ShaderData -> this serializes the shader data to/from disk
	ShaderPlatformData -> platform specific
*/

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"

enum ShaderType
{
	ShaderType_Geometry,
	ShaderType_Vertex,
	ShaderType_Pixel
};

struct ShaderAssetCreateParams : public AssetCreateParams
{
	ShaderType type;
};

// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct ShaderAssetData : public AssetData
{
public:
	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams* params);

	MemBlock spvData;
};

// shader is the game facing class that represents any type of shader  (geometry, vertex, pixel)
class Shader : public Resource
{
	void OnAssetDeliver(struct AssetData* data);
	virtual void Reload() override;
	virtual AssetType GetAssetType() const override { return AssetType_Shader; }
	ShaderType m_shaderType;
	ShaderAssetData* m_assetData;
	struct ShaderPlatformData* m_platformData;

public:
	Shader(const string& name, ShaderType shaderType);
	virtual ~Shader();

	ShaderAssetData* GetAssetData() { return m_assetData; }
	ShaderPlatformData* GetPlatformData() { return m_platformData; }
};

// texture factory keeps a map of all the currently created textures
class ShaderFactory : public Module<ShaderFactory>
{
	map<u64, Shader*> m_resources;

public:
	ShaderFactory();

	Shader* Create(const string& name, ShaderType shaderType);
	void Destroy(Shader* shader);
};

// texture references is a scoped pointer to a texture.  the texture will be destroyed with the TextureRef goes out of scope or is destroyed
// you can create differnet types of textures through this using
//
// ShaderRef myShader;
// myTex->CreateShader("myPixelShader", ShaderType_Pixel);
//
class ShaderRef : public ResourceRef<Shader, ShaderFactory>
{
public:
	void Create(const string &name, ShaderType shaderType)
	{
		Destroy();
		m_ptr = ShaderFactory::Instance().Create(name, shaderType);
		ShaderFactory::Instance().Create(name, shaderType);
	}
};

