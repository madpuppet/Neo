#include "Neo.h"
#include "RenderPass.h"
#include "RenderThread.h"

#define RENDERPASS_VERSION 1

DECLARE_MODULE(RenderPassFactory, NeoModuleInitPri_RenderPassFactory, NeoModulePri_None, NeoModulePri_None);

RenderPass::RenderPass(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_Texture, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
}

RenderPass::~RenderPass()
{
}

void RenderPass::OnAssetDeliver(AssetData* data)
{
	if (data)
	{
		Assert(data->type == AssetType_RenderPass, "Bad Asset Type");
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

RenderPassFactory::RenderPassFactory()
{
	auto ati = new AssetTypeInfo();
	ati->name = "RenderPass";
	ati->assetExt = ".neorp";
	ati->assetCreator = []() -> AssetData* { return new RenderPassAssetData; };
	ati->sourceExt.push_back({ { ".renderpass" }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(AssetType_RenderPass, ati);
}

RenderPass* RenderPassFactory::Create(const string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		RenderPass* resource = new RenderPass(name);
		m_resources.insert(std::pair<u64, RenderPass*>(hash, resource));

		return resource;
	}
	it->second->IncRef();
	return it->second;
}

void RenderPassFactory::Destroy(RenderPass* texture)
{
	if (texture && texture->DecRef() == 0)
	{
		u64 hash = StringHash64(texture->GetName());
		m_resources.erase(hash);
		delete texture;
	}
}

bool RenderPassAssetData::SrcFilesToAsset(vector<MemBlock>& srcFiles, AssetCreateParams* params)
{
	return true;
}

MemBlock RenderPassAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(AssetType_RenderPass);
	stream.WriteU16(RENDERPASS_VERSION);
	stream.WriteString(name);

	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool RenderPassAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	type = (AssetType)stream.ReadU16();
	version = stream.ReadU16();
	name = stream.ReadString();

	if (type != AssetType_RenderPass)
	{
		LOG(RenderPass, STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_RenderPass));
		return false;
	}
	if (version != RENDERPASS_VERSION)
	{
		LOG(RenderPass, STR("Rebuilding {} - old version {} - expected {}", name, version, RENDERPASS_VERSION));
		return false;
	}

	return true;
}
