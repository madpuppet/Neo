#include "Neo.h"
#include "Material.h"
#include "GILThread.h"
#include "StringUtils.h"
#include "SHAD.h"

#define MATERIAL_VERSION 1

DECLARE_MODULE(MaterialFactory, NeoModulePri_MaterialFactory);

Material::Material(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_Material, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
}

Material::Material(const string& name, Material *parent) : Resource(name)
{
	
}


Material::~Material()
{
}

void Material::OnAssetDeliver(AssetData* data)
{
	Assert(data->type == AssetType_Material, "Bad Asset Type");
	m_assetData = dynamic_cast<MaterialAssetData*>(data);

	// create dependant resources
	vector<Resource*> dependantResources;
	for (auto &mode : m_assetData->modes)
	{
		mode.second->pixelShader.Create(mode.second->pixelShaderName, ShaderType_Pixel);
		mode.second->vertexShader.Create(mode.second->vertexShaderName, ShaderType_Vertex);
		dependantResources.push_back(*mode.second->pixelShader);
		dependantResources.push_back(*mode.second->pixelShader);
	}

	// create material platform data once all dependant resources are loaded

	GILThread::Instance().AddNonRenderTask([this]() { m_platformData = MaterialPlatformData_Create(m_assetData); OnLoadComplete(); });
}

void Material::Reload()
{
}

MaterialFactory::MaterialFactory()
{
	auto ati = new AssetTypeInfo();
	ati->m_assetCreateFromData = [](MemBlock memBlock) -> AssetData* { auto assetData = new MaterialAssetData; assetData->MemoryToAsset(memBlock); return assetData; };
	ati->m_assetCreateFromSource = [](const vector<MemBlock>& srcBlocks, AssetCreateParams* params) -> AssetData* { return MaterialAssetData::Create(srcBlocks, params);  };
	ati->m_assetExt = ".neomat";
	ati->m_sourceExt.push_back({ { ".material" }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(AssetType_Material, ati);
}

Material* MaterialFactory::Create(const string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		Material* resource = new Material(name);
		m_resources.insert(std::pair<u64, Material*>(hash, resource));

		return resource;
	}
	it->second->IncRef();
	return it->second;
}

Material* MaterialFactory::CreateInstance(const string& name, const string& original)
{
	// create parent
	Material* parent = Create(original);
	u64 hash = StringHash64(original);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		// parent doesn't exist.. create it
		Material* resource = new Material(name);
		m_resources.insert(std::pair<u64, Material*>(hash, resource));
		return resource;
	}


	// clone parent (when finished)
	if (!StringEqual(it->second->GetName(), original))
	{
	}

	it->second->IncRef();
	return it->second;
}

void MaterialFactory::Destroy(Material* resource)
{
	if (resource && resource->DecRef() == 0)
	{
		u64 hash = StringHash64(resource->GetName());
		m_resources.erase(hash);
		delete resource;
	}
}

static vector<string> s_blendModeNames = { "opaque", "alpha", "additive", "subractive", "darken", "fade" };
static vector<string> s_cullModeNames = { "none", "front", "back" };
static vector<string> s_samplerFilterNames = { "nearest", "linear", "nearestMipNearest", "nearestMipLinear", "linearMipNearest", "linearMipLinear"};
static vector<string> s_samplerWrapNames = { "clamp", "repeat" };
static vector<string> s_samplerCompareNames = { "none", "gequal", "lequal" };

AssetData* MaterialAssetData::Create(vector<MemBlock> srcFiles, AssetCreateParams* params)
{
	auto asset = new MaterialAssetData;
	Assert(srcFiles.size() == 1, STR("Expected 1 src file for material"));

	auto shad = new SHADReader("material", (const char*)srcFiles[0].Mem(), (int)srcFiles[0].Size());
	auto rootChildren = shad->root->GetChildren();
	for (auto modeNode : rootChildren)
	{
		Assert(modeNode->IsName("mode"), STR("Expected 'mode' sections only, got {}", modeNode->Name()));
		auto mode = new MaterialMode;
		mode->name = modeNode->GetString(1);
		u64 modeHash = StringHash64(mode->name);
		auto modeChildren = modeNode->GetChildren();
		for (auto fieldNode : modeChildren)
		{
			if (fieldNode->IsName("vertexShader"))
			{
				mode->vertexShaderName = fieldNode->GetString();
			}
			else if (fieldNode->IsName("pixelShader"))
			{
				mode->pixelShaderName = fieldNode->GetString();
			}
			else if (fieldNode->IsName("blend"))
			{
				mode->blendMode = (MaterialBlendMode)fieldNode->GetEnum(s_blendModeNames);
			}
			else if (fieldNode->IsName("cull"))
			{
				mode->cullMode = (MaterialCullMode)fieldNode->GetEnum(s_cullModeNames);
			}
			else if (fieldNode->IsName("zread"))
			{
				mode->zread = fieldNode->GetBool();
			}
			else if (fieldNode->IsName("zwrite"))
			{
				mode->zwrite = fieldNode->GetBool();
			}
			else if (fieldNode->IsName("uniforms"))
			{
				auto uniformNodes = fieldNode->GetChildren();
				for (auto uniformNode : uniformNodes)
				{
					if (uniformNode->IsName("texture"))
					{
						auto uniform = new MaterialUniform_Texture(uniformNode->GetString());
						mode->uniforms.push_back(uniform);
						auto textureNodes = uniformNode->GetChildren();
						for (auto textureNode : textureNodes)
						{
							if (textureNode->IsName("image"))
							{
								uniform->textureName = textureNode->GetString();
							}
							else if (textureNode->IsName("filter"))
							{
								uniform->minFilter = (SamplerFilter)textureNode->GetEnum(s_samplerFilterNames, 0);
								uniform->magFilter = uniform->minFilter;
								if (textureNode->GetValueCount() == 2)
								{
									uniform->magFilter = (SamplerFilter)textureNode->GetEnum(s_samplerFilterNames, 1);
								}
							}
							else if (textureNode->IsName("wrap"))
							{
								uniform->uWrap = (SamplerWrap)textureNode->GetEnum(s_samplerWrapNames, 0);
								uniform->vWrap = uniform->uWrap;
								if (textureNode->GetValueCount() == 2)
								{
									uniform->vWrap = (SamplerWrap)textureNode->GetEnum(s_samplerWrapNames, 1);
								}
							}
							else if (textureNode->IsName("compare"))
							{
								uniform->compare = (SamplerCompare)textureNode->GetEnum(s_samplerCompareNames);
							}
						}
					}
					else if (uniformNode->IsName("vec4"))
					{
						auto uniform = new MaterialUniform_Vec4(uniformNode->GetString());
						mode->uniforms.push_back(uniform);
						auto uniformChildren = uniformNode->GetChildren();
						for (auto node : uniformChildren)
						{
							if (node->IsName("default"))
							{
								uniform->value = node->GetVector4();
							}
						}
					}
					else if (uniformNode->IsName("f32"))
					{
						auto uniform = new MaterialUniform_F32(uniformNode->GetString());
						mode->uniforms.push_back(uniform);
						auto uniformChildren = uniformNode->GetChildren();
						for (auto node : uniformChildren)
						{
							if (node->IsName("default"))
							{
								uniform->value = node->GetF32();
							}
						}
					}
					else if (uniformNode->IsName("i32"))
					{
						auto uniform = new MaterialUniform_I32(uniformNode->GetString());
						mode->uniforms.push_back(uniform);
						auto uniformChildren = uniformNode->GetChildren();
						for (auto node : uniformChildren)
						{
							if (node->IsName("default"))
							{
								uniform->value = node->GetI32();
							}
						}
					}
				}
			}
		}
		for (int i = 0; i < modeNode->GetValueCount(); i++)
		{
			asset->modes[StringHash64(modeNode->GetString(i))] = mode;
		}
	}

	return asset;
}

MemBlock MaterialAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(AssetType_Material);
	stream.WriteU16(MATERIAL_VERSION);
	stream.WriteString(name);

	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool MaterialAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	type = (AssetType)stream.ReadU16();
	version = stream.ReadU16();
	name = stream.ReadString();

	if (type != AssetType_Material)
	{
		Log(STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_Material));
		return false;
	}
	if (version != MATERIAL_VERSION)
	{
		Log(STR("Rebuilding {} - old version {} - expected {}", name, version, MATERIAL_VERSION));
		return false;
	}

	return true;
}
