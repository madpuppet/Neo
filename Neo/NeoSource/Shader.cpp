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
hashtable<string, SAType> SAType_Lookup;
hashtable<SAType, string> SATypeToString_Lookup;
hashtable<string, SAFmt> SAFmt_Lookup;
hashtable<SAType, SAFmt> SATypeToSAFmt_Lookup;

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
		string saTypeStr = tokens[1];
		string name = tokens[2];
		SAType saType = SAType_Lookup[saTypeStr];
		SAFmt saFmt;
		if (tokens.size() >= 5)
		{
			string fmtStr = tokens[3];
			saFmt = SAFmt_Lookup[fmtStr];
		}
		else
		{
			saFmt = SATypeToSAFmt_Lookup[saType];
		}
		int binding = (int)attribs.size();
		attribs.emplace_back(name, saType, saFmt, binding);
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
			SROs.emplace_back(name, type, sroSet, sroBinding, stageMask, uboInfo);
		}

		// VS_IN <type> <name>(<binding>)
		else if (tokens[0] == "VS_IN")
		{
			if (tokens.size() >= 3)
			{
				ParseAttributes(vertexAttributes, tokens);
			}
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
						sro.set, sro.binding, sro.uboInfo->structName, bodyStr, sro.uboInfo->varName);

					if (sro.stageMask & SROStage_Vertex)
						vertOutputFile << out;
					if (sro.stageMask & SROStage_Fragment)
						fragOutputFile << out;
				}
				break;

			case SROType_Sampler:
				{
					string out = std::format("layout(set = {}, binding = {}) uniform sampler2D {};\n\n", sro.set, sro.binding, sro.name);
					if (sro.stageMask & SROStage_Vertex)
						vertOutputFile << out;
					if (sro.stageMask & SROStage_Fragment)
						fragOutputFile << out;
				}
				break;
		}
	}

	// output vertex shader attributes
	for (auto& attrib : vertexAttributes)
	{
		string attribType = SATypeToString_Lookup[attrib.type];
		vertOutputFile << std::format("layout(location = {}) in {} {};\n", attrib.binding, attribType, attrib.name);
	}

	// output interpolants
	for (auto& attrib : interpolants)
	{
		string attribType = SATypeToString_Lookup[attrib.type];
		vertOutputFile << std::format("layout(location = {}) out {} {};\n", attrib.binding, attribType, attrib.name);
		fragOutputFile << std::format("layout(location = {}) in {} {};\n", attrib.binding, attribType, attrib.name);
	}

	// output fragment shader outs
	for (auto& attrib : fragmentOutputs)
	{
		string attribType = SATypeToString_Lookup[attrib.type];
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
		stream.WriteU32(attrib.fmt);
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
		attrib.type = (SAType)stream.ReadU32();
		attrib.fmt = (SAFmt)stream.ReadU32();
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

	WriteAttributes(stream, vertexAttributes);
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

	ReadAttributes(stream, vertexAttributes);
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

ShaderFactory::ShaderFactory()
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

	SAType_Lookup["f32"] = SAType_f32;
	SAType_Lookup["i32"] = SAType_i32;
	SAType_Lookup["vec2"] = SAType_vec2;
	SAType_Lookup["vec3"] = SAType_vec3;
	SAType_Lookup["vec4"] = SAType_vec4;
	SAType_Lookup["ivec2"] = SAType_ivec2;
	SAType_Lookup["ivec3"] = SAType_ivec3;
	SAType_Lookup["ivec4"] = SAType_ivec4;

	
	SATypeToString_Lookup[SAType_f32] = "f32";
	SATypeToString_Lookup[SAType_i32] = "i32";
	SATypeToString_Lookup[SAType_vec2] = "vec2";
	SATypeToString_Lookup[SAType_vec3] = "vec3";
	SATypeToString_Lookup[SAType_vec4] = "vec4";
	SATypeToString_Lookup[SAType_ivec2] = "ivec2";
	SATypeToString_Lookup[SAType_ivec3] = "ivec3";
	SATypeToString_Lookup[SAType_ivec4] = "ivec4";

	SAFmt_Lookup["R32_SFLOAT"] = SAFmt_R32_SFLOAT;
	SAFmt_Lookup["R32G32_SFLOAT"] = SAFmt_R32G32_SFLOAT;
	SAFmt_Lookup["R32G32B32_SFLOAT"] = SAFmt_R32G32B32_SFLOAT;
	SAFmt_Lookup["R32G32B32A32_SFLOAT"] = SAFmt_R32G32B32A32_SFLOAT;
	SAFmt_Lookup["R32_SINT"] = SAFmt_R32_SINT;
	SAFmt_Lookup["R32G32_SINT"] = SAFmt_R32G32_SINT;
	SAFmt_Lookup["R32G32B32_SINT"] = SAFmt_R32G32B32_SINT;
	SAFmt_Lookup["R32G32B32A32_SINT"] = SAFmt_R32G32B32A32_SINT;
	SAFmt_Lookup["R8G8B8A8_UNORM"] = SAFmt_R8G8B8A8_UNORM;

	SATypeToSAFmt_Lookup[SAType_f32] = SAFmt_R32_SFLOAT;
	SATypeToSAFmt_Lookup[SAType_i32] = SAFmt_R32_SINT;
	SATypeToSAFmt_Lookup[SAType_vec2] = SAFmt_R32G32_SFLOAT;
	SATypeToSAFmt_Lookup[SAType_vec3] = SAFmt_R32G32B32_SFLOAT;
	SATypeToSAFmt_Lookup[SAType_vec4] = SAFmt_R32G32B32A32_SFLOAT;
	SATypeToSAFmt_Lookup[SAType_ivec2] = SAFmt_R32G32_SINT;
	SATypeToSAFmt_Lookup[SAType_ivec3] = SAFmt_R32G32B32_SINT;
	SATypeToSAFmt_Lookup[SAType_ivec4] = SAFmt_R32G32B32A32_SINT;
}

Shader* ShaderFactory::Create(const string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		Shader* resource = new Shader(name);
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

