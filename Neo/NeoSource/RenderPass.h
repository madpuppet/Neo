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

	struct AttachmentInfo
	{
		string name;
		TexturePixelFormat fmt;
		TextureRef texture;
	};
	vector<AttachmentInfo> colorAttachments;
	AttachmentInfo depthAttachment;
	ivec2 size;
};

// render pass is a set of color & depth attachments for outputting to
class RenderPass : public Resource
{
	virtual void Reload() override;
	RenderPassAssetData* m_assetData;
	struct RenderPassPlatformData* m_platformData;

public:
	static const string AssetType;
	virtual const string& GetType() const { return AssetType; }
	virtual ~RenderPass() {}
	void OnAssetDeliver(struct AssetData* data);

	void Apply();

	RenderPassAssetData* GetAssetData() { return m_assetData; }
	RenderPassPlatformData* GetPlatformData() { return m_platformData; }
};

class RenderPassFactory : public ResourceFactory<RenderPass>, public Module<RenderPassFactory> {};
using RenderPassRef = ResourceRef<RenderPass, RenderPassFactory>;


