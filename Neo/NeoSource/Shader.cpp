#include "Neo.h"
#include "Shader.h"
#include "StringUtils.h"
#include "RenderThread.h"

#define SHADER_VERSION 1

DECLARE_MODULE(ShaderFactory, NeoModulePri_ShaderFactory);

bool ShaderAssetData::SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams *params)
{
	spvData = std::move(srcFiles[0]);
	return true;
}

MemBlock ShaderAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(AssetType_Shader);
	stream.WriteU16(SHADER_VERSION);
	stream.WriteString(name);
	stream.WriteMemory(spvData);
	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool ShaderAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	type = (AssetType)stream.ReadU16();
	version = stream.ReadU16();
	name = stream.ReadString();

	if (type != AssetType_Shader)
	{
		Log(STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_Shader));
		return false;
	}
	if (version != SHADER_VERSION)
	{
		Log(STR("Rebuilding {} - old version {} - expected {}", name, version, SHADER_VERSION));
		return false;
	}

	spvData = stream.ReadMemory();

	return true;
}

Shader::Shader(const string& name, ShaderType shaderType) : Resource(name)
{
	auto params = new ShaderAssetCreateParams{ {}, shaderType };
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_Shader, name, params, [this](AssetData* data) { OnAssetDeliver(data); });
}

Shader::~Shader()
{
}

void Shader::OnAssetDeliver(struct AssetData* data)
{
	if (data)
	{
		m_assetData = dynamic_cast<ShaderAssetData*>(data);
		RenderThread::Instance().AddPreDrawTask([this]() { m_platformData = ShaderPlatformData_Create(m_assetData); OnLoadComplete(); });
	}
	else
	{
		m_failedToLoad = true;
		OnLoadComplete();
	}
}

void Shader::Reload()
{
}

ShaderFactory::ShaderFactory()
{
	auto ati = new AssetTypeInfo();
	ati->m_assetCreator = []() -> AssetData* { return new ShaderAssetData; };
	ati->m_assetExt = ".neoshd";
	ati->m_sourceExt.push_back({ { ".spv" }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(AssetType_Shader, ati);
}

Shader* ShaderFactory::Create(const string& name, ShaderType shaderType)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		Shader* resource = new Shader(name, shaderType);
		m_resources.insert(std::pair<u64, Shader*>(hash, resource));
		return resource;
	}
	it->second->IncRef();
	return it->second;
}

void ShaderFactory::Destroy(Shader* shader)
{
	if (shader && shader->DecRef() == 0)
	{
		u64 hash = StringHash64(shader->GetName());
		m_resources.erase(hash);
		delete shader;
	}
}

