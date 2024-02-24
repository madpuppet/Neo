#include "Neo.h"
#include "Shader.h"
#include "StringUtils.h"
#include "RenderThread.h"
#include "ShaderManager.h"

#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#define SHADER_VERSION 1

hashtable<string, SROType> SROType_Lookup;
hashtable<string, SROStage> SROStage_Lookup;

bool CompileShader(MemBlock srcBlock, MemBlock& spvBlock, AssetType assetType);

DECLARE_MODULE(ShaderFactory, NeoModuleInitPri_ShaderFactory, NeoModulePri_None, NeoModulePri_None);

bool LoadFile(const string& path, MemBlock& block)
{
	// load the compiled file back in
	std::ifstream inFile(path, std::ios::binary | std::ios::ate);
	if (!inFile.is_open())
		return false;

	std::streamsize fileSize = inFile.tellg();
	inFile.seekg(0, std::ios::beg);
	block.Resize(fileSize);
	inFile.read(reinterpret_cast<char*>(block.Mem()), block.Size());
	inFile.close();
	return true;
}

void ParseAttributes(vector<ShaderAssetData::ShaderAttribute> &attribs, stringlist tokens)
{
	if (tokens.size() >= 3)
	{
		string vertAttribTypeStr = tokens[1];
		string name = tokens[2];
		VertAttribType vertAttribType = VertAttribType_Lookup[vertAttribTypeStr];
		int binding = (int)attribs.size();
		attribs.emplace_back(name, vertAttribType, binding);
	}
	else
	{
		Error("Bad Attribute Line in shader");
	}
}

bool ShaderAssetData::SrcFilesToAsset(vector<MemBlock>& srcFiles, AssetCreateParams* params)
{
	auto& sm = ShaderManager::Instance();

	// parse the shader file before building vertex & fragment shaders
	string srcStr((char*)srcFiles[0].Mem(), srcFiles[0].Size());
	std::istringstream iss(srcStr);
	string line;
	string code[3];
	int scanningCode = -1;
	while (std::getline(iss, line))
	{
		stringlist tokens = StringSplitIntoTokens(line);
		if (tokens.size() < 1)
			continue;

		// get the SRO objects... (ubo, sampler, ubod, sbo)
		auto uboInfo = sm.FindUBO(tokens[0]);
		bool isSampler = StringEqual(tokens[0], "Sampler");
		if (uboInfo || isSampler)
		{
			Assert(tokens.size() > 6, "Not enough tokens for a UBO declaration!");
			string varName = std::move(tokens[1]);
			int sroSet = StringToI32(tokens[3]);
			int sroBinding = StringToI32(tokens[5]);
			u32 stageMask = 0;
			string stageMaskStr = (tokens.size() > 8) ? tokens[8] : "";
			for (auto c : stageMaskStr)
			{
				switch (c)
				{
					case 'F':
						stageMask |= SROStage_Fragment;
						break;
					case 'V':
						stageMask |= SROStage_Vertex;
						break;
					case 'G':
						stageMask |= SROStage_Geometry;
						break;
				}
			}
			if (!stageMask)
				stageMask = SROStage_Vertex | SROStage_Fragment;

			SROType type = uboInfo ? SROType_UBO : SROType_Sampler;
			SROs.emplace_back(tokens[0], varName, type, sroSet, sroBinding, stageMask, uboInfo);
		}

		// VS_IN <type> <name>(<binding>)
		else if (tokens[0] == "VS_IN")
		{
			Assert(tokens.size() >= 2, "Not enough tokens!");
			inputAttributesName = tokens[1];
			iad = ShaderManager::Instance().FindIAD(inputAttributesName);
			Assert(iad, STR("Cannot find input attributes {}", inputAttributesName));
		}
		else if (tokens[0] == "VS_TO_FS")
		{
			ParseAttributes(interpolants, tokens);
		}
		else if (tokens[0] == "PS_OUT")
		{
			ParseAttributes(fragmentOutputs, tokens);
		}
		else if (tokens[0] == "COMMON_CODE")
		{
			scanningCode = 0;
		}
		else if (tokens[0] == "VERTEX_SHADER_CODE")
		{
			scanningCode = 1;
		}
		else if (tokens[0] == "FRAGMENT_SHADER_CODE")
		{
			scanningCode = 2;
		}
		else if (scanningCode >= 0)
		{
			code[scanningCode] += line;
		}
	}

	// Get the standard temporary file location
	std::filesystem::path tempDir = std::filesystem::temp_directory_path();

	// Create a temporary file path
	std::filesystem::path tempFilePath = tempDir / "tempfile";

	static int count = 0;
	string vertShaderPath = std::format("{}_{}.vert", tempFilePath.string(), name);
	string fragShaderPath = std::format("{}_{}.frag", tempFilePath.string(), name);

	string vertSpvPath = std::format("{}_{}_vert.spv", tempFilePath.string(), name);
	string fragSpvPath = std::format("{}_{}_frag.spv", tempFilePath.string(), name);

	string vertErrPath = std::format("{}_{}_vert.err", tempFilePath.string(), name);
	string fragErrPath = std::format("{}_{}_frag.err", tempFilePath.string(), name);

	// output the vertex & fragment files
	std::ofstream vertOutputFile(vertShaderPath, std::ios::out | std::ios::binary);
	std::ofstream fragOutputFile(fragShaderPath, std::ios::out | std::ios::binary);

	vertOutputFile << "#version 450\n\n";
	fragOutputFile << "#version 450\n\n";

	// output SRO bindings
	for (auto& sro : SROs)
	{
		switch (sro.type)
		{
			case SROType_UBO:
				{
					auto bodyStr = sm.UBOContentsToString(*sro.uboInfo);

					string out = std::format("layout(set = {}, binding = {}) uniform {}\n{{\n{}}} {};\n\n",
						sro.set, sro.binding, sro.uboInfo->structName, bodyStr, sro.varName);

					if (sro.stageMask & SROStage_Vertex)
						vertOutputFile << out;
					if (sro.stageMask & SROStage_Fragment)
						fragOutputFile << out;
				}
				break;

			case SROType_Sampler:
				{
					string out = std::format("layout(set = {}, binding = {}) uniform sampler2D {};\n\n", sro.set, sro.binding, sro.varName);
					if (sro.stageMask & SROStage_Vertex)
						vertOutputFile << out;
					if (sro.stageMask & SROStage_Fragment)
						fragOutputFile << out;
				}
				break;
		}
	}

	// output vertex shader attributes
	for (auto& attrib : iad->attributes)
	{
		string attribType = VertexFormatToString(attrib.format);
		vertOutputFile << std::format("layout(location = {}) in {} {};\n", attrib.location, attribType, attrib.name);
	}

	// output interpolants
	for (auto& attrib : interpolants)
	{
		string attribType = VertAttribTypeToString_Lookup[attrib.type];
		vertOutputFile << std::format("layout(location = {}) out {} {};\n", attrib.binding, attribType, attrib.name);
		fragOutputFile << std::format("layout(location = {}) in {} {};\n", attrib.binding, attribType, attrib.name);
	}

	// output fragment shader outs
	for (auto& attrib : fragmentOutputs)
	{
		string attribType = VertAttribTypeToString_Lookup[attrib.type];
		fragOutputFile << std::format("layout(location = {}) out {} {};\n", attrib.binding, attribType, attrib.name);
	}

	// output vertex shader code
	vertOutputFile << code[0] << std::endl << code[1] << std::endl;
	vertOutputFile.close();

	// output the fragment shader code
	fragOutputFile << code[0] << std::endl << code[2] << std::endl;
	fragOutputFile.close();

	string vertCommand = "glslc -fshader-stage=vertex " + vertShaderPath + " -o " + vertSpvPath + " 2>" + vertErrPath;
	string fragCommand = "glslc -fshader-stage=fragment " + fragShaderPath + " -o " + fragSpvPath + " 2>" + fragErrPath;
	std::system(vertCommand.c_str());
	std::system(fragCommand.c_str());

	// load the compiled file back in
	if (!LoadFile(vertSpvPath, vertSPVData))
	{
		MemBlock errBlock;
		if (LoadFile(vertErrPath, errBlock))
		{
			string err((const char *)errBlock.Mem(), errBlock.Size());
			Error(STR("Failed to compile {}:\n{}", vertSpvPath, err));
		}
		else
		{
			Error(STR("Failed to load spv: {}", vertSpvPath));
		}
	}
	if (!LoadFile(fragSpvPath, fragSPVData))
	{
		MemBlock errBlock;
		if (LoadFile(fragErrPath, errBlock))
		{
			string err((const char*)errBlock.Mem(), errBlock.Size());
			Error(STR("Failed to compile {}:\n{}", fragSpvPath, err));
		}
		else
		{
			Error(STR("Failed to load spv: {}", fragSpvPath));
		}
	}

	return true;
}

void WriteAttributes(Serializer& stream, vector<ShaderAssetData::ShaderAttribute>& attribs)
{
	stream.WriteU32((u32)attribs.size());
	for (auto& attrib : attribs)
	{
		stream.WriteString(attrib.name);
		stream.WriteU32(attrib.type);
		stream.WriteU32(attrib.binding);
	}
}

void ReadAttributes(Serializer& stream, vector<ShaderAssetData::ShaderAttribute>& attribs)
{
	attribs.clear();
	u32 size = stream.ReadU32();
	for (u32 i = 0; i < size; i++)
	{
		ShaderAssetData::ShaderAttribute attrib;
		attrib.name = stream.ReadString();
		attrib.type = (VertAttribType)stream.ReadU32();
		attrib.binding = stream.ReadU32();
	}
}

MemBlock ShaderAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(type);
	stream.WriteU16(AssetType_Shader);
	stream.WriteString(name);

	stream.WriteU32((u32)SROs.size());
	for (auto& sro : SROs)
	{
		stream.WriteString(sro.name);
		stream.WriteU32(sro.type);
		stream.WriteU32(sro.set);
		stream.WriteU32(sro.binding);
		stream.WriteU32(sro.stageMask);
	}

	stream.WriteString(inputAttributesName);

	WriteAttributes(stream, interpolants);
	WriteAttributes(stream, fragmentOutputs);

	stream.WriteMemory(vertSPVData);
	stream.WriteMemory(fragSPVData);
	
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
		LOG(Shader, STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_Shader));
		return false;
	}
	if (version != SHADER_VERSION)
	{
		LOG(Shader, STR("Rebuilding {} - old version {} - expected {}", name, version, SHADER_VERSION));
		return false;
	}
	
	auto& sm = ShaderManager::Instance();
	u32 sroSize = stream.ReadU32();
	SROs.clear();
	for (u32 i = 0; i < sroSize; i++)
	{
		ShaderResourceObjectInfo sro;
		sro.name = stream.ReadString();
		sro.type = (SROType)stream.ReadU32();
		sro.set = stream.ReadU32();
		sro.binding = stream.ReadU32();
		sro.stageMask = stream.ReadU32();
		sro.uboInfo = sm.FindUBO(sro.name);
		SROs.push_back(sro);
	}
	auto sortFunc = [](const ShaderResourceObjectInfo& a, const ShaderResourceObjectInfo& b) { return (a.set < b.set) || (a.binding < b.binding); };
	std::sort(SROs.begin(), SROs.end(), sortFunc);

	inputAttributesName = stream.ReadString();
	iad = ShaderManager::Instance().FindIAD(inputAttributesName);

	ReadAttributes(stream, interpolants);
	ReadAttributes(stream, fragmentOutputs);

	vertSPVData = stream.ReadMemory();
	fragSPVData = stream.ReadMemory();

	return true;
}

Shader::Shader(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_Shader, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
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

template <> ResourceFactory<Shader>::ResourceFactory()
{
	auto ati = new AssetTypeInfo();
	ati->name = "Shader";
	ati->assetCreator = []() -> AssetData* { return new ShaderAssetData; };
	ati->assetExt = ".neoshader";
	ati->sourceExt.push_back({ { ".shader" }, true });		// compile from source
	AssetManager::Instance().RegisterAssetType(AssetType_Shader, ati);

	SROStage_Lookup["Geometry"] = SROStage_Geometry;
	SROStage_Lookup["Vertex"] = SROStage_Vertex;
	SROStage_Lookup["Fragment"] = SROStage_Fragment;
}
