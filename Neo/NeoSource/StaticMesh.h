#pragma once

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"
#include "Material.h"

class StaticMesh : public Resource
{
	virtual void Reload() override;

	struct StaticMeshAssetData* m_assetData;
	struct StaticMeshPlatformData* m_platformData;

public:
	static const string AssetType;
	virtual const string& GetType() const { return AssetType; }
	virtual ~StaticMesh() {}

	void OnAssetDeliver(struct AssetData* data);

	StaticMeshAssetData* GetAssetData() { return m_assetData; }
	StaticMeshPlatformData* GetPlatformData() { return m_platformData; }
};

// texture factory keeps a map of all the currently created textures

class StaticMeshFactory : public ResourceFactory<StaticMesh>, public Module<StaticMeshFactory> {};
using StaticMeshRef = ResourceRef<StaticMesh, StaticMeshFactory>;

// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct StaticMeshAssetData : public AssetData
{
	~StaticMeshAssetData() {}

	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams* params) override;

	vector<Vertex_p3f_t2f_c4b> verts;
	vector<u32> indices;
	string materialName;

	MaterialRef material;
};

