#include "Neo.h"
#include "RenderScene.h"
#include "RenderThread.h"
#include "SHAD.h"
#include "ResourceLoadedManager.h"

#define RENDERSCENE_VERSION 1

DECLARE_MODULE(RenderSceneFactory, NeoModuleInitPri_RenderSceneFactory, NeoModulePri_None, NeoModulePri_None);

const string RenderScene::AssetType = "RenderScene";

void RenderScene::OnAssetDeliver(AssetData* data)
{
	auto assetData = (RenderSceneAssetData*)data;
	if (data)
	{
		vector<Resource*> dependantResources;
		for (auto &stage : assetData->renderStages)
		{
			stage.renderPass.Create(stage.renderPassName);
			dependantResources.emplace_back(stage.renderPass);
		}

		m_assetData = dynamic_cast<RenderSceneAssetData*>(data);

		// we need to wait for our dependant resources, like Shaders and Textures,  to load first before creating our platform data (which are pipeline states)
		// note that if they are already loaded, this will just trigger off the callback immediately
		ResourceLoadedManager::Instance().AddDependancyList(this, dependantResources, [this]() { OnLoadComplete(); });
	}
	else
	{
		m_failedToLoad = true;
		OnLoadComplete();
	}
}

void RenderScene::Reload()
{
}

template <> ResourceFactory<RenderScene>::ResourceFactory()
{
	auto ati = new AssetTypeInfo();
	ati->name = RenderScene::AssetType;
	ati->assetExt = ".neorscn";
	ati->assetCreator = []() -> AssetData* { return new RenderSceneAssetData; };
	ati->sourceExt.push_back({ { ".renderscene" }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(ati);
}

bool RenderSceneAssetData::SrcFilesToAsset(vector<MemBlock>& srcFiles, AssetCreateParams* params)
{
	auto shad = new SHADReader("renderscene", (const char*)srcFiles[0].Mem(), (int)srcFiles[0].Size());
	auto rootChildren = shad->root->GetChildren();
	for (auto fieldNode : rootChildren)
	{
		if (fieldNode->IsName("renderpass"))
		{
			renderStages.emplace_back(fieldNode->GetString());
		}
	}
	delete shad;
	return true;
}

MemBlock RenderSceneAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(RENDERSCENE_VERSION);
	stream.WriteString(name);
	stream.WriteU32((u32)renderStages.size());
	for (auto& stage : renderStages)
	{
		stream.WriteString(stage.renderPassName);
	}
	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool RenderSceneAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	version = stream.ReadU16();
	name = stream.ReadString();

	if (version != RENDERSCENE_VERSION)
	{
		LOG(RenderPass, STR("Rebuilding {} - old version {} - expected {}", name, version, RENDERSCENE_VERSION));
		return false;
	}

	u32 count = stream.ReadU32();
	renderStages.resize(count);
	for (u32 i = 0; i < count; i++)
	{
		renderStages[i].renderPassName = stream.ReadString();
	}
	return true;
}

void RenderScene::Execute()
{
	if (IsLoaded())
	{
		for (auto& stage : m_assetData->renderStages)
		{
			stage.renderPass->Apply();
			stage.renderPass->ExecuteTasks();
		}
	}
}

void RenderScene::Apply()
{
	RenderThread::Instance().SetRenderScene(this);
}