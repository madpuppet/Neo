#pragma once

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"
#include "Shader.h"
#include "Material.h"

class Model : public Resource
{
	void OnAssetDeliver(struct AssetData* data);
	virtual AssetType GetAssetType() const override { return AssetType_Model; }

	virtual void Reload() override;

	struct ModelAssetData* m_assetData;
	struct ModelPlatformData* m_platformData;

public:
	Model(const string& name);
	Model(const string& name, Material* parent);
	virtual ~Model();

	ModelAssetData* GetAssetData() { return m_assetData; }
	ModelPlatformData* GetPlatformData() { return m_platformData; }
};

// texture factory keeps a map of all the currently created textures
class ModelFactory : public Module<ModelFactory>
{
	map<u64, Model*> m_resources;

public:
	ModelFactory();

	Model* Create(const string& name);
	void Destroy(Model* resource);
};

using ModelRef = ResourceRef<Model, ModelFactory>;

// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct ModelAssetData : public AssetData
{
	~ModelAssetData() {}

	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(const vector<MemBlock> &srcFiles, AssetCreateParams* params) override;

	struct Vertex
	{
		vec3 pos;
		vec3 col;
		vec2 uv;
		bool operator==(const Vertex& other) const {
			return pos == other.pos && col == other.col && uv == other.uv;
		}
	};
	vector<Vertex> verts;
	vector<u32> indices;
	string materialName;

	MaterialRef material;
};

namespace std {
	template<> struct hash<ModelAssetData::Vertex> {
		size_t operator()(ModelAssetData::Vertex const& vertex) const {
			return ((hash<vec3>()(vertex.pos) ^ (hash<vec3>()(vertex.col) << 1)) >> 1) ^ (hash<vec2>()(vertex.uv) << 1);
		}
	};
}

