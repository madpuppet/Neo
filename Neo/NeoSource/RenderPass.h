#pragma once

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"

// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct RenderPassAssetData : public AssetData
{
public:
	~RenderPassAssetData() {}

	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock>& srcBlocks, struct AssetCreateParams* params) override;

	vector<string> colorAttachments;
	string depthAttachment;

	// resolved textures
	vector<TextureRef> textures;
};

// render pass is a set of color & depth attachments for outputting to
class RenderPass : public Resource
{
	void OnAssetDeliver(struct AssetData* data);

	virtual void Reload() override;
	virtual AssetType GetAssetType() const override { return AssetType_RenderPass; }

	RenderPassAssetData* m_assetData;
	struct RenderPassPlatformData* m_platformData;

public:
	RenderPass(const string& name);
	virtual ~RenderPass();

	RenderPassAssetData* GetAssetData() { return m_assetData; }
	RenderPassPlatformData* GetPlatformData() { return m_platformData; }
};

class RenderPassFactory : public Module<RenderPassFactory>
{
	hashtable<u64, RenderPass*> m_resources;

public:
	RenderPassFactory();

	RenderPass* Create(const string& name);

	void Destroy(RenderPass* texture);
};



#if 0 //TODO: refactor Factory to be a template
template <class T>
class Factory : public Module<Factory<T>>
{
	hashtable<u64, T*> m_resources;

public:
	Factory();
	T* Create(const string& name)
	{
		u64 hash = StringHash64(name);
		auto it = m_resources.find(hash);
		if (it == m_resources.end())
		{
			T* resource = new T(name);
			m_resources.insert(std::pair<u64, T*>(hash, resource));

			return resource;
		}
		it->second->IncRef();
		return it->second;
	}
	void Destroy(T* resource)
	{
		if (resource && resource->DecRef() == 0)
		{
			u64 hash = StringHash64(resource->GetName());
			m_resources.erase(hash);
			delete resource;
		}
	}
};
Factory<RenderPass>::Factory<RenderPass>()
{
	auto ati = new AssetTypeInfo();
	ati->name = "RenderPass";
	ati->assetExt = ".neorp";
	ati->assetCreator = []() -> AssetData* { return new RenderPassAssetData; };
	ati->sourceExt.push_back({ { ".renderpass" }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(AssetType_RenderPass, ati);
}
using RenderPassRef = ResourceRef<RenderPass, Factory<RenderPass>>;
#endif


using RenderPassRef = ResourceRef<RenderPass, RenderPassFactory>;


