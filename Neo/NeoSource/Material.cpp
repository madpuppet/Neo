#include "Neo.h"
#include "Material.h"
#include "StringUtils.h"
#include "SHAD.h"
#include "ResourceLoadedManager.h"

#define MATERIAL_VERSION 1

DECLARE_MODULE(MaterialFactory, NeoModuleInitPri_MaterialFactory, NeoModulePri_None, NeoModulePri_None);

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
	if (data)
	{
		Assert(data->type == AssetType_Material, "Bad Asset Type");
		m_assetData = dynamic_cast<MaterialAssetData*>(data);

		// create dependant resources
		vector<Resource*> dependantResources;
		m_assetData->pixelShader.Create(m_assetData->pixelShaderName);
		m_assetData->vertexShader.Create(m_assetData->vertexShaderName);
		dependantResources.push_back(*m_assetData->pixelShader);
		dependantResources.push_back(*m_assetData->vertexShader);
		for (auto uniform : m_assetData->uniforms)
		{
			if (uniform->type == UniformType_Texture)
			{
				auto uniformTexture = dynamic_cast<MaterialUniform_Texture*>(uniform);
				uniformTexture->texture.Create(uniformTexture->textureName);
				dependantResources.push_back(*uniformTexture->texture);
			}
		}

		LOG(Material, STR("Adding Dependancy List: {}", dependantResources.size()));

		// we need to wait for our dependant resources, like Shaders and Textures,  to load first before creating our platform data (which are pipeline states)
		// note that if they are already loaded, this will just trigger off the callback immediately
		ResourceLoadedManager::Instance().AddDependancyList(this, dependantResources, [this]() { m_platformData = MaterialPlatformData_Create(m_assetData); OnLoadComplete(); });
	}
	else
	{
		// failed to load but we still need to make it as loaded so dependant resources can continue
		m_failedToLoad = true;
		OnLoadComplete();
	}

}

void Material::Reload()
{
}

MaterialFactory::MaterialFactory()
{
	auto ati = new AssetTypeInfo();
	ati->m_assetCreator = []() -> AssetData* { return new MaterialAssetData; };
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

bool MaterialAssetData::SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams* params)
{
	Assert(srcFiles.size() == 1, STR("Expected 1 src file for material"));

	auto shad = new SHADReader("material", (const char*)srcFiles[0].Mem(), (int)srcFiles[0].Size());
	auto rootChildren = shad->root->GetChildren();
	for (auto fieldNode : rootChildren)
	{
		if (fieldNode->IsName("vertexShader"))
		{
			vertexShaderName = fieldNode->GetString();
		}
		else if (fieldNode->IsName("pixelShader"))
		{
			pixelShaderName = fieldNode->GetString();
		}
		else if (fieldNode->IsName("blend"))
		{
			blendMode = (MaterialBlendMode)fieldNode->GetEnum(s_blendModeNames);
		}
		else if (fieldNode->IsName("cull"))
		{
			cullMode = (MaterialCullMode)fieldNode->GetEnum(s_cullModeNames);
		}
		else if (fieldNode->IsName("zread"))
		{
			zread = fieldNode->GetBool();
		}
		else if (fieldNode->IsName("zwrite"))
		{
			zwrite = fieldNode->GetBool();
		}
		else if (fieldNode->IsName("uniforms"))
		{
			auto uniformNodes = fieldNode->GetChildren();
			for (auto uniformNode : uniformNodes)
			{
				if (uniformNode->IsName("texture"))
				{
					auto uniform = new MaterialUniform_Texture(uniformNode->GetString());
					uniforms.push_back(uniform);
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
					uniforms.push_back(uniform);
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
					uniforms.push_back(uniform);
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
					uniforms.push_back(uniform);
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

	return true;
}

MemBlock MaterialAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(AssetType_Material);
	stream.WriteU16(MATERIAL_VERSION);
	stream.WriteString(name);

	stream.WriteU8((u8)blendMode);
	stream.WriteU8((u8)cullMode);
	stream.WriteBool(zread);
	stream.WriteBool(zwrite);
	stream.WriteString(vertexShaderName);
	stream.WriteString(pixelShaderName);
	stream.WriteU16((u16)uniforms.size());
	for (auto uniform : uniforms)
	{	
		stream.WriteU8(uniform->type);
		stream.WriteString(uniform->uniformName);
		switch (uniform->type)
		{
			case UniformType_Texture:
			{
				auto uniformTexture = dynamic_cast<MaterialUniform_Texture*>(uniform);
				stream.WriteString(uniformTexture->textureName);
				stream.WriteU8(uniformTexture->minFilter);
				stream.WriteU8(uniformTexture->magFilter);
				stream.WriteU8(uniformTexture->uWrap);
				stream.WriteU8(uniformTexture->vWrap);
				stream.WriteU8(uniformTexture->compare);
			}
			break;
			case UniformType_Vec4:
			{
				auto uniformVec4 = dynamic_cast<MaterialUniform_Vec4*>(uniform);
				stream.WriteVec4(uniformVec4->value);
			}
			break;
			case UniformType_F32:
			{
				auto uniformF32 = dynamic_cast<MaterialUniform_F32*>(uniform);
				stream.WriteF32(uniformF32->value);
			}
			break;
			case UniformType_I32:
			{
				auto uniformI32 = dynamic_cast<MaterialUniform_I32*>(uniform);
				stream.WriteI32(uniformI32->value);
			}
			break;
		}
	}

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
		LOG(Material, STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_Material));
		return false;
	}
	if (version != MATERIAL_VERSION)
	{
		LOG(Material, STR("Rebuilding {} - old version {} - expected {}", name, version, MATERIAL_VERSION));
		return false;
	}

	blendMode = (MaterialBlendMode)stream.ReadU8();
	cullMode = (MaterialCullMode)stream.ReadU8();
	zread = stream.ReadBool();
	zwrite = stream.ReadBool();
	vertexShaderName = stream.ReadString();
	pixelShaderName = stream.ReadString();

	u16 uniformsCount = stream.ReadU16();
	for (u16 u = 0; u < uniformsCount; u++)
	{
		auto type = (UniformType)stream.ReadU8();
		auto uniformName = stream.ReadString();
		switch (type)
		{
			case UniformType_Texture:
			{
				auto uniform = new MaterialUniform_Texture(uniformName);
				uniform->textureName = stream.ReadString();
				uniform->minFilter = (SamplerFilter)stream.ReadU8();
				uniform->magFilter = (SamplerFilter)stream.ReadU8();
				uniform->uWrap = (SamplerWrap)stream.ReadU8();
				uniform->vWrap = (SamplerWrap)stream.ReadU8();
				uniform->compare = (SamplerCompare)stream.ReadU8();
				uniforms.push_back(uniform);
			}
			break;
			case UniformType_Vec4:
			{
				auto uniform = new MaterialUniform_Vec4(uniformName);
				uniform->value = stream.ReadVec4();
				uniforms.push_back(uniform);
			}
			break;
			case UniformType_I32:
			{
				auto uniform = new MaterialUniform_I32(uniformName);
				uniform->value = stream.ReadI32();
				uniforms.push_back(uniform);
			}
			break;
			case UniformType_F32:
			{
				auto uniform = new MaterialUniform_F32(uniformName);
				uniform->value = stream.ReadF32();
				uniforms.push_back(uniform);
			}
			break;
		}
	}

	return true;
}


