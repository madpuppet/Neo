#pragma once

/*
* Shader Resource
* Declare 3 factories,  since we want to have the same asset name for different shader types (vertex,pixel,geometry,compute)
*/

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"
#include "StringUtils.h"

struct ShaderPlatformData;

enum SROType
{
	SROType_Unknown=-1,

	SROType_UBO,
	SROType_Sampler
};

enum SROStage
{
	SROStage_Geometry = 1,
	SROStage_Vertex = 2,
	SROStage_Fragment = 4
};
extern hashtable<string, SROStage> SROStage_Lookup;

// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct ShaderAssetData : public AssetData
{
public:
	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock>& srcFiles, AssetCreateParams* params);

	MemBlock vertSPVData;
	MemBlock fragSPVData;

	struct ShaderResourceObjectInfo
	{
		string name;		// name of resource
		string varName;		// variable name of this ubo
		SROType type;		// ubo/sampler/ubod/sbo
		u32 set;			// 0..4
		u32 binding;		// 0..x
		u32 stageMask;		// vertex/pixel

		UBOInfo* uboInfo;		// resolved UBO
	};
	vector<ShaderResourceObjectInfo> SROs;

	struct ShaderAttribute
	{
		string name;
		VertAttribType type;			// f32, i32, vec2, vec3, vec4, ivec2, ivec3, ivec4
		u32 binding;
	};
	
	string inputAttributesName;
	InputAttributesDescription* iad = nullptr;

	vector<ShaderAttribute> interpolants;
	vector<ShaderAttribute> fragmentOutputs;
};

class Shader : public Resource
{
	void OnAssetDeliver(struct AssetData* data);
	virtual void Reload() override;
	virtual AssetType GetAssetType() const override { return AssetType_Shader; }
	ShaderAssetData* m_assetData;
	ShaderPlatformData* m_platformData;

public:
	Shader(const string& name);
	virtual ~Shader();

	ShaderAssetData* GetAssetData() { return m_assetData; }
	ShaderPlatformData* GetPlatformData() { return m_platformData; }
};

class ShaderFactory : public Module<ShaderFactory>
{
	map<u64, Shader*> m_resources;

public:
	ShaderFactory();

	Shader* Create(const string& name);
	void Destroy(Shader* shader);
};

using ShaderRef = ResourceRef<Shader, ShaderFactory>;


