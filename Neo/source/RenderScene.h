#pragma once

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"

struct RenderSceneAssetData;

// render pass is a set of color & depth attachments for outputting to
class RenderScene : public Resource
{
	virtual void Reload() override;
	RenderSceneAssetData* m_assetData = nullptr;

public:
	static const string AssetType;
	virtual const string& GetType() const { return AssetType; }
	virtual ~RenderScene() {}
	void OnAssetDeliver(struct AssetData* data);

	// make this the active render scene
	void Apply();

	// execute the render scene - sets each renderpass and executes each task of each renderpass
	void Execute();

	RenderSceneAssetData* GetAssetData() { return m_assetData; }
};

class RenderSceneFactory : public ResourceFactory<RenderScene>, public Module<RenderSceneFactory> {};
using RenderSceneRef = ResourceRef<RenderScene, RenderSceneFactory>;

struct RenderSceneStage
{
	string renderPassName;
	RenderPassRef renderPass;
};

struct RenderSceneAssetData : public AssetData
{
public:
	~RenderSceneAssetData() {}

	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock>& srcBlocks, struct AssetCreateParams* params) override;

	vector<RenderSceneStage> renderStages;
};
