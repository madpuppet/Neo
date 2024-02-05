#include "Neo.h"
#include "VertexShader.h"
#include "StringUtils.h"
#include "RenderThread.h"

#define SHADER_VERSION 1

bool CompileShader(MemBlock srcBlock, MemBlock& spvBlock, AssetType assetType);

DECLARE_MODULE(VertexShaderFactory, NeoModuleInitPri_VertexShaderFactory, NeoModulePri_None, NeoModulePri_None);

bool VertexShaderAssetData::SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams *params)
{
	return CompileShader(srcFiles[0], spvData, AssetType_VertexShader);
}

MemBlock VertexShaderAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(type);
	stream.WriteU16(AssetType_VertexShader);
	stream.WriteString(name);
	stream.WriteMemory(spvData);
	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool VertexShaderAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	type = (AssetType)stream.ReadU16();
	version = stream.ReadU16();
	name = stream.ReadString();

	if (type != AssetType_VertexShader)
	{
		Log(STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_VertexShader));
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

VertexShader::VertexShader(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_VertexShader, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
}

VertexShader::~VertexShader()
{
}

void VertexShader::OnAssetDeliver(struct AssetData* data)
{
	if (data)
	{
		m_assetData = dynamic_cast<VertexShaderAssetData*>(data);
		RenderThread::Instance().AddPreDrawTask([this]() { m_platformData = VertexShaderPlatformData_Create(m_assetData); OnLoadComplete(); });
	}
	else
	{
		m_failedToLoad = true;
		OnLoadComplete();
	}
}

void VertexShader::Reload()
{
}

VertexShaderFactory::VertexShaderFactory()
{
	auto ati = new AssetTypeInfo();
	ati->m_assetCreator = []() -> AssetData* { return new VertexShaderAssetData; };
	ati->m_assetExt = ".neovsh";
	ati->m_sourceExt.push_back({ { ".vsh" }, true });		// compile from source
	AssetManager::Instance().RegisterAssetType(AssetType_VertexShader, ati);
}

VertexShader* VertexShaderFactory::Create(const string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		VertexShader* resource = new VertexShader(name);
		m_resources.insert(std::pair<u64, VertexShader*>(hash, resource));
		return resource;
	}
	it->second->IncRef();
	return it->second;
}

void VertexShaderFactory::Destroy(VertexShader* shader)
{
	if (shader && shader->DecRef() == 0)
	{
		u64 hash = StringHash64(shader->GetName());
		m_resources.erase(hash);
		delete shader;
	}
}



#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdlib>

bool CompileShader(MemBlock srcBlock, MemBlock &spvBlock, AssetType assetType)
{
	// Get the standard temporary file location
	std::filesystem::path tempDir = std::filesystem::temp_directory_path();

	// Create a temporary file path
	std::filesystem::path tempFilePath = tempDir / "tempfile";

	static int count = 0;
	string srcPath = std::format("{}{}.shader", tempFilePath.string(), ++count);
	string spvPath = std::format("{}{}.spv", tempFilePath.string(), ++count);
	string errPath = std::format("{}{}.err", tempFilePath.string(), ++count);

	// write shader file
	std::ofstream outFile(srcPath, std::ios::binary);
	if (outFile.is_open())
	{
		outFile.write(reinterpret_cast<const char*>(srcBlock.Mem()), srcBlock.Size());
		outFile.close();
	}
	else 
	{
		Error(STR("Failed to open shader temp file for writing: {}", srcPath));
		return false;
	}

	// compile it
	string stage;
	switch (assetType)
	{
		case AssetType_VertexShader:
			stage = "-fshader-stage=vertex ";
			break;
		case AssetType_PixelShader:
			stage = "-fshader-stage=fragment ";
			break;
		default:
			Error("Unsupported asset type for shader compile!");
			break;
	}

	// compile the file
	std::string command = "glslc " + stage + srcPath + " -o " + spvPath + " 2>" + errPath;
	int result = std::system(command.c_str());
	if (result != 0) 
	{
		Error(STR("Failed to compile shader: {}", command));
		return false;
	}

	// load the compiled file back in
	std::ifstream inFile(spvPath, std::ios::binary | std::ios::ate);
	if (!inFile.is_open())
	{
		Error(STR("Failed to load spv: {}", spvPath));
		return false;
	}
	std::streamsize fileSize = inFile.tellg();
	inFile.seekg(0, std::ios::beg);
	spvBlock.Resize(fileSize);
	inFile.read(reinterpret_cast<char*>(spvBlock.Mem()), spvBlock.Size());
	inFile.close();

	// all done!
	return true;
}

