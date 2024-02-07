#include "Neo.h"
#include "PixelShader.h"
#include "StringUtils.h"
#include "RenderThread.h"

#define SHADER_VERSION 1

bool CompileShader(MemBlock srcBlock, MemBlock& spvBlock, AssetType assetType);

DECLARE_MODULE(PixelShaderFactory, NeoModuleInitPri_PixelShaderFactory, NeoModulePri_None, NeoModulePri_None);

bool PixelShaderAssetData::SrcFilesToAsset(vector<MemBlock>& srcFiles, AssetCreateParams* params)
{
	return CompileShader(srcFiles[0], spvData, AssetType_PixelShader);
}

MemBlock PixelShaderAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(type);
	stream.WriteU16(AssetType_PixelShader);
	stream.WriteString(name);
	stream.WriteMemory(spvData);
	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool PixelShaderAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	type = (AssetType)stream.ReadU16();
	version = stream.ReadU16();
	name = stream.ReadString();

	if (type != AssetType_PixelShader)
	{
		LOG(Shader, STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_PixelShader));
		return false;
	}
	if (version != SHADER_VERSION)
	{
		LOG(Shader, STR("Rebuilding {} - old version {} - expected {}", name, version, SHADER_VERSION));
		return false;
	}

	spvData = stream.ReadMemory();

	return true;
}

PixelShader::PixelShader(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_PixelShader, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
}

PixelShader::~PixelShader()
{
}

void PixelShader::OnAssetDeliver(struct AssetData* data)
{
	if (data)
	{
		m_assetData = dynamic_cast<PixelShaderAssetData*>(data);
		RenderThread::Instance().AddPreDrawTask([this]() { m_platformData = PixelShaderPlatformData_Create(m_assetData); OnLoadComplete(); });
	}
	else
	{
		m_failedToLoad = true;
		OnLoadComplete();
	}
}

void PixelShader::Reload()
{
}

PixelShaderFactory::PixelShaderFactory()
{
	auto ati = new AssetTypeInfo();
	ati->name = "PixelShader";
	ati->assetExt = ".neopsh";
	ati->assetCreator = []() -> AssetData* { return new PixelShaderAssetData; };
	ati->sourceExt.push_back({ { ".psh" }, true });		// compile from source
	AssetManager::Instance().RegisterAssetType(AssetType_PixelShader, ati);
}

PixelShader* PixelShaderFactory::Create(const string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		PixelShader* resource = new PixelShader(name);
		m_resources.insert(std::pair<u64, PixelShader*>(hash, resource));
		return resource;
	}
	it->second->IncRef();
	return it->second;
}

void PixelShaderFactory::Destroy(PixelShader* shader)
{
	if (shader && shader->DecRef() == 0)
	{
		u64 hash = StringHash64(shader->GetName());
		m_resources.erase(hash);
		delete shader;
	}
}
