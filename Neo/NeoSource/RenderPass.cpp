#include "Neo.h"
#include "RenderPass.h"
#include "RenderThread.h"

#define RENDERPASS_VERSION 1

DECLARE_MODULE(RenderPassFactory, NeoModuleInitPri_RenderPassFactory, NeoModulePri_None, NeoModulePri_None);

const string RenderPass::AssetType = "RenderPass";

void RenderPass::OnAssetDeliver(AssetData* data)
{
	if (data)
	{
		m_assetData = dynamic_cast<RenderPassAssetData*>(data);
		RenderThread::Instance().AddPreDrawTask([this]() { m_platformData = RenderPassPlatformData_Create(m_assetData); OnLoadComplete(); });
	}
	else
	{
		m_failedToLoad = true;
		OnLoadComplete();
	}
}

void RenderPass::Reload()
{
}

template <> ResourceFactory<RenderPass>::ResourceFactory()
{
	auto ati = new AssetTypeInfo();
	ati->name = RenderPass::AssetType;
	ati->assetExt = ".neorp";
	ati->assetCreator = []() -> AssetData* { return new RenderPassAssetData; };
	ati->sourceExt.push_back({ { ".renderpass" }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(ati);
}


bool RenderPassAssetData::SrcFilesToAsset(vector<MemBlock>& srcFiles, AssetCreateParams* params)
{
	return true;
}

MemBlock RenderPassAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(RENDERPASS_VERSION);
	stream.WriteString(name);

	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool RenderPassAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	version = stream.ReadU16();
	name = stream.ReadString();

	if (version != RENDERPASS_VERSION)
	{
		LOG(RenderPass, STR("Rebuilding {} - old version {} - expected {}", name, version, RENDERPASS_VERSION));
		return false;
	}

	return true;
}
