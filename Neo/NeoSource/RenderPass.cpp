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
			if (!colorAttach.useSwapChain)
			{
				LOG(RenderPass, STR("Create Color Render Target {} - {}x{} - format {}", colorAttach.name, assetData->size.x, assetData->size.y, (int)colorAttach.fmt));
				colorAttach.texture.CreateRenderTarget(colorAttach.name, assetData->size.x, assetData->size.y, colorAttach.fmt);
				dependantResources.emplace_back(colorAttach.texture);
			}
		}
		if (assetData->depthAttachment.name.empty() && !assetData->depthAttachment.useSwapChain)
		{
			LOG(RenderPass, STR("Create Depth Render Target {} - {}x{} - format {}", assetData->depthAttachment.name, assetData->size.x, assetData->size.y, (int)assetData->depthAttachment.fmt));
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
		else if (fieldNode->IsName("viewport"))
		{
			viewportRect.x = fieldNode->GetF32(0);
			viewportRect.y = fieldNode->GetF32(1);
			viewportRect.w = fieldNode->GetF32(2);
			viewportRect.h = fieldNode->GetF32(3);
			viewportMinDepth = fieldNode->GetF32(4);
			viewportMaxDepth = fieldNode->GetF32(5);
		}
		else if (fieldNode->IsName("scissor"))
		{
			scissorRect.x = fieldNode->GetF32(0);
			scissorRect.y = fieldNode->GetF32(1);
			scissorRect.w = fieldNode->GetF32(2);
			scissorRect.h = fieldNode->GetF32(3);
		}
		else if (fieldNode->IsName("color"))
		{
			bool linear = false;
			vec4 clearColor{ 0,0,0,1 };
			bool doClear = false;
			for (int i = 0; i < fieldNode->GetChildCount(); i++)
			{
				auto paramsNode = fieldNode->GetChild(i);
				if (paramsNode->IsName("colorspace"))
				{
					linear = (paramsNode->GetString() == "linear");
				}
				else if (paramsNode->IsName("clear"))
				{
					clearColor = paramsNode->GetVector4();
					doClear = true;
				}
			}

			//todo: format might be RGBA on other platforms??
			TexturePixelFormat fmt = linear ? PixFmt_B8G8R8A8_UNORM : PixFmt_B8G8R8A8_SRGB;
			string name = fieldNode->GetString();
			bool useSwapChain = (name == "swapchain");
			AttachmentInfo ai;
			ai.useSwapChain = useSwapChain;
			ai.name = name;
			ai.fmt = fmt;
			ai.clear.color = clearColor;
			ai.doClear = doClear;
			colorAttachments.push_back(ai);
		}
		else if (fieldNode->IsName("depth"))
		{
			TexturePixelFormat fmt = PixFmt_D24_UNORM_S8_UINT;
			DepthClear depthClear{ 1, 0 };
			bool doClear = false;
			for (int i = 0; i < fieldNode->GetChildCount(); i++)
			{
				auto paramsNode = fieldNode->GetChild(i);
				if (paramsNode->IsName("clear"))
				{
					depthClear.depth = paramsNode->GetF32(0);
					depthClear.stencil = (u32)paramsNode->GetI32(1);
					doClear = true;
				}
			}
			depthAttachment.name = fieldNode->GetString();
			depthAttachment.useSwapChain = depthAttachment.name == "swapchain";
			depthAttachment.fmt = fmt;
			depthAttachment.clear.depth = depthClear;
			depthAttachment.doClear = doClear;
		}
	}
	delete shad;
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
		stream.WriteBool(attach.useSwapChain);
		stream.WriteU32(attach.fmt);
		stream.WriteVec4(attach.clear.color);
		stream.WriteBool(attach.doClear);
	}
	stream.WriteString(depthAttachment.name);
	stream.WriteBool(depthAttachment.useSwapChain);
	stream.WriteU32(depthAttachment.fmt);
	stream.WriteF32(depthAttachment.clear.depth.depth);
	stream.WriteU32(depthAttachment.clear.depth.stencil);
	stream.WriteBool(depthAttachment.doClear);

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
		bool useSwapChain = stream.ReadBool();
		TexturePixelFormat fmt = (TexturePixelFormat)stream.ReadU32();
		vec4 color = stream.ReadVec4();
		bool doClear = stream.ReadBool();

		AttachmentInfo ai;
		ai.name = name;
		ai.useSwapChain = useSwapChain;
		ai.clear.color = color;
		ai.fmt = fmt;
		ai.doClear = doClear;
		colorAttachments.push_back(ai);
	}

	depthAttachment.name = stream.ReadString();
	depthAttachment.useSwapChain = stream.ReadBool();
	depthAttachment.fmt = (TexturePixelFormat)stream.ReadU32();
	depthAttachment.clear.depth = { stream.ReadF32(),  stream.ReadU32() };
	depthAttachment.doClear = stream.ReadBool();
	return true;
}

void RenderPass::Apply()
{
	GIL::Instance().SetRenderPass(this);
	ExecuteTasks();
}

int RenderPass::AddTask(GenericCallback task, int priority)
{
	ScopedMutexLock lock(m_taskLock);
	int handle = m_nextTaskHandle++;
	auto taskBundle = new TaskBundle{ handle, priority, task };
	m_tasks.push_back(taskBundle);
	return handle;
}

void RenderPass::RemoveTask(int handle)
{
	ScopedMutexLock lock(m_taskLock);
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it)
	{
		if ((*it)->handle == handle)
		{
			delete *it;
			m_tasks.erase(it);
		}
	}
}

void RenderPass::ExecuteTasks()
{
	m_taskLock.Lock();
	vector<TaskBundle*> doTasks = m_tasks;
	m_taskLock.Release();

	for (auto task : doTasks)
		task->task();
}

