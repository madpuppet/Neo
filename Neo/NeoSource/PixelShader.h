#pragma once

/*
* Shader Resource
* Declare 3 factories,  since we want to have the same asset name for different shader types (vertex,pixel,geometry,compute)
*/

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"
#include "StringUtils.h"

struct PixelShaderPlatformData;

// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct PixelShaderAssetData : public AssetData
{
public:
	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock>& srcFiles, AssetCreateParams* params);
	MemBlock spvData;
};

class PixelShader : public Resource
{
	void OnAssetDeliver(struct AssetData* data);
	virtual void Reload() override;
	virtual AssetType GetAssetType() const override { return AssetType_PixelShader; }
	PixelShaderAssetData* m_assetData;
	PixelShaderPlatformData* m_platformData;

public:
	PixelShader(const string& name);
	virtual ~PixelShader();

	PixelShaderAssetData* GetAssetData() { return m_assetData; }
	PixelShaderPlatformData* GetPlatformData() { return m_platformData; }
};

class PixelShaderFactory : public Module<PixelShaderFactory>
{
	map<u64, PixelShader*> m_resources;

public:
	PixelShaderFactory();

	PixelShader* Create(const string& name);
	void Destroy(PixelShader* shader);
};

using PixelShaderRef = ResourceRef<PixelShader, PixelShaderFactory>;
