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

	ivec2 size{ 0,0 };
	rect viewportRect{ 0,0,1,1 };
	f32 viewportMinDepth;
	f32 viewportMaxDepth;
	rect scissorRect{ 0,0,1,1 };
	struct DepthClear
	{
		f32 depth;
		u32 stencil;
	};
	struct AttachmentInfo
	{
		bool useSwapChain;
		string name;
		TexturePixelFormat fmt;
		union
		{
			vec4 color;
			DepthClear depth;
		} clear;
		bool doClear;
		TextureRef texture;
	};
	vector<AttachmentInfo> colorAttachments;
	AttachmentInfo depthAttachment;
};

// render pass is a set of color & depth attachments for outputting to
class RenderPass : public Resource
{
	virtual void Reload() override;
	RenderPassAssetData* m_assetData;
	struct RenderPassPlatformData* m_platformData;
	Mutex m_taskLock;
	int m_nextTaskHandle = 0;
	vector<TaskBundle*> m_tasks;

public:
	static const string AssetType;
	virtual const string& GetType() const { return AssetType; }
	virtual ~RenderPass() {}
	void OnAssetDeliver(struct AssetData* data);

	void Apply();

	// add task - returns a handle
	int AddTask(GenericCallback task, int priority=0);
	void RemoveTask(int handle);

	void ExecuteTasks();

	RenderPassAssetData* GetAssetData() { return m_assetData; }
	RenderPassPlatformData* GetPlatformData() { return m_platformData; }
};

class RenderPassFactory : public ResourceFactory<RenderPass>, public Module<RenderPassFactory> {};
using RenderPassRef = ResourceRef<RenderPass, RenderPassFactory>;


