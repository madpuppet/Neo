#pragma once

/*
* Shader Resource
* Declare 3 factories,  since we want to have the same asset name for different shader types (vertex,pixel,geometry,compute)
*/

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"
#include "StringUtils.h"

struct VertexShaderPlatformData;

// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct VertexShaderAssetData : public AssetData
{
public:
	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams* params);
	MemBlock spvData;
};

class VertexShader : public Resource
{
	void OnAssetDeliver(struct AssetData* data);
	virtual void Reload() override;
	virtual AssetType GetAssetType() const override { return AssetType_VertexShader; }
	VertexShaderAssetData* m_assetData;
	VertexShaderPlatformData* m_platformData;

public:
	VertexShader(const string& name);
	virtual ~VertexShader();

	VertexShaderAssetData* GetAssetData() { return m_assetData; }
	VertexShaderPlatformData* GetPlatformData() { return m_platformData; }
};

class VertexShaderFactory : public Module<VertexShaderFactory>
{
	map<u64, VertexShader*> m_resources;

public:
	VertexShaderFactory();

	VertexShader* Create(const string& name);
	void Destroy(VertexShader* shader);
};

using VertexShaderRef = ResourceRef<VertexShader, VertexShaderFactory>;
