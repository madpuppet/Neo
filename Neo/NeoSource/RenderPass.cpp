#include "Neo.h"
#include "RenderPass.h"
#include "RenderThread.h"
#include "SHAD.h"
#include "ResourceLoadedManager.h"

#define RENDERPASS_VERSION 1

DECLARE_MODULE(RenderPassFactory, NeoModuleInitPri_RenderPassFactory, NeoModulePri_None, NeoModulePri_None);

const string RenderPass::AssetType = "RenderPass";

void RenderPass::OnAssetDeliver(AssetData* data)
{
	auto assetData = (RenderPassAssetData*)data;
	if (data)
	{
		vector<Resource*> dependantResources;
		for (auto& colorAttach : assetData->colorAttachments)
		{
			if (colorAttach.name != "default")
			{
				colorAttach.texture.CreateRenderTarget(colorAttach.name, assetData->size.x, assetData->size.y, colorAttach.fmt);
				dependantResources.emplace_back(colorAttach.texture);
			}
		}
		if (!assetData->depthAttachment.name.empty() && assetData->depthAttachment.name != "default")
		{
			assetData->depthAttachment.texture.CreateRenderTarget(assetData->depthAttachment.name, assetData->size.x, assetData->size.y, assetData->depthAttachment.fmt);
			dependantResources.emplace_back(assetData->depthAttachment.texture);
		}
		m_assetData = dynamic_cast<RenderPassAssetData*>(data);

		// we need to wait for our dependant resources, like Shaders and Textures,  to load first before creating our platform data (which are pipeline states)
		// note that if they are already loaded, this will just trigger off the callback immediately
		ResourceLoadedManager::Instance().AddDependancyList(this, dependantResources, [this]() { m_platformData = RenderPassPlatformData_Create(m_assetData); OnLoadComplete(); });
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
	auto shad = new SHADReader("renderpass", (const char*)srcFiles[0].Mem(), (int)srcFiles[0].Size());
	auto rootChildren = shad->root->GetChildren();
	for (auto fieldNode : rootChildren)
	{
		if (fieldNode->IsName("size"))
		{
			size = fieldNode->GetVector2i();
		}
		if (fieldNode->IsName("color"))
		{
			bool linear = false;
			for (int i = 0; i < fieldNode->GetChildCount(); i++)
			{
				auto paramsNode = fieldNode->GetChild(i);
				if (paramsNode->IsName("colorspace"))
				{
					linear = (paramsNode->GetString() == "linear");
				}
			}

			//todo: format might be RGBA on other platforms??
			TexturePixelFormat fmt = linear ? PixFmt_B8G8R8A8_UNORM : PixFmt_B8G8R8A8_SRGB;
			colorAttachments.emplace_back(fieldNode->GetString(), fmt);
		}
		else if (fieldNode->IsName("depth"))
		{
			TexturePixelFormat fmt = PixFmt_D24_UNORM_S8_UINT;
			depthAttachment = { fieldNode->GetString(), fmt };
		}
	}
	return true;
}

MemBlock RenderPassAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(RENDERPASS_VERSION);
	stream.WriteString(name);

	stream.WriteI32(size.x);
	stream.WriteI32(size.y);
	stream.WriteU32((u32)colorAttachments.size());
	for (auto& attach : colorAttachments)
	{
		stream.WriteString(attach.name);
		stream.WriteU32(attach.fmt);
	}
	stream.WriteString(depthAttachment.name);
	stream.WriteU32(depthAttachment.fmt);

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

	size.x = stream.ReadI32();
	size.y = stream.ReadI32();
	u32 count = stream.ReadU32();
	for (u32 i = 0; i < count; i++)
	{
		string name = stream.ReadString();
		TexturePixelFormat fmt = (TexturePixelFormat)stream.ReadU32();
		colorAttachments.emplace_back(name,fmt);
	}

	string depthName = stream.ReadString();
	TexturePixelFormat depthFmt = (TexturePixelFormat)stream.ReadU32();
	depthAttachment = { depthName,depthFmt };
	return true;
}

void RenderPass::Apply()
{
	GIL::Instance().SetRenderPass(this);
}


