#include "Neo.h"
#include "Material.h"
#include "StringUtils.h"
#include "SHAD.h"
#include "ResourceLoadedManager.h"
#include "ShaderManager.h"

#define MATERIAL_VERSION 2

DECLARE_MODULE(MaterialFactory, NeoModuleInitPri_MaterialFactory, NeoModulePri_None, NeoModulePri_None);

const char* UniformTypeToString[] = { "vec4", "ivec4", "mat4x4", "f32", "i32" };

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
		m_assetData->shader.Create(m_assetData->shaderName);
		dependantResources.push_back(*m_assetData->shader);
		for (auto sampler : m_assetData->samplers)
		{
			sampler->texture.Create(sampler->textureName);
			dependantResources.push_back(*sampler->texture);
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
	ati->name = "Material";
	ati->assetCreator = []() -> AssetData* { return new MaterialAssetData; };
	ati->assetExt = ".neomat";
	ati->sourceExt.push_back({ { ".material" }, true });		// on of these src image files
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

static vector<string> s_blendModeNames = { "opaque", "alpha", "blend", "additive", "subtractive" };
static vector<string> s_cullModeNames = { "none", "front", "back" };
static vector<string> s_samplerFilterNames = { "nearest", "linear" };
static vector<string> s_samplerWrapNames = { "clamp", "repeat" };
static vector<string> s_samplerCompareNames = { "none", "gequal", "lequal" };

UBOMemberInfo* FindUniformMember(UBOInfo* ubo, const string &name)
{
	for (auto& member : ubo->members)
	{
		if (member.name == name)
			return &member;
	}
	return nullptr;
}

mat4x4 MakeMatrix(const vec3& pyr, const vec3& scale, const vec3& pos)
{
	// Convert Euler angles to radians
	vec3 eulerRad(glm::radians(pyr.x), glm::radians(pyr.y), glm::radians(pyr.z));

	// Create the rotation matrix
	mat4x4 rotationMatrix = glm::eulerAngleXYZ(eulerRad.x, eulerRad.y, eulerRad.z);

	// Create the translation matrix
	mat4x4 translationMatrix = glm::translate(glm::mat4(1.0f), pos);

	// Create the scale matrix
	mat4x4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	// Combine all matrices to get the final transformation matrix
	return translationMatrix * rotationMatrix * scaleMatrix;
}

bool MaterialAssetData::SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams* params)
{
	Assert(srcFiles.size() == 1, STR("Expected 1 src file for material"));

	auto shad = new SHADReader("material", (const char*)srcFiles[0].Mem(), (int)srcFiles[0].Size());
	auto rootChildren = shad->root->GetChildren();
	for (auto fieldNode : rootChildren)
	{
		if (fieldNode->IsName("shader"))
		{
			shaderName = fieldNode->GetString();
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
		else if (fieldNode->IsName("sampler"))
		{
			auto sampler = new MaterialSampler;
			sampler->samplerName = fieldNode->GetString();
			samplers.push_back(sampler);

			auto textureNodes = fieldNode->GetChildren();
			for (auto textureNode : textureNodes)
			{
				if (textureNode->IsName("image"))
				{
					sampler->textureName = textureNode->GetString();
				}
				else if (textureNode->IsName("filter"))
				{
					sampler->mipFilter = sampler->magFilter = sampler->minFilter = (SamplerFilter)textureNode->GetEnum(s_samplerFilterNames, 0);
					if (textureNode->GetValueCount() > 1)
					{
						sampler->magFilter = (SamplerFilter)textureNode->GetEnum(s_samplerFilterNames, 1);
						if (textureNode->GetValueCount() > 2)
						{
							sampler->mipFilter = (SamplerFilter)textureNode->GetEnum(s_samplerFilterNames, 2);
						}
					}
				}
				else if (textureNode->IsName("wrap"))
				{
					sampler->vWrap = sampler->uWrap = (SamplerWrap)textureNode->GetEnum(s_samplerWrapNames, 0);
					if (textureNode->GetValueCount() > 1)
					{
						sampler->vWrap = (SamplerWrap)textureNode->GetEnum(s_samplerWrapNames, 1);
					}
				}
				else if (textureNode->IsName("compare"))
				{
					sampler->compare = (SamplerCompare)textureNode->GetEnum(s_samplerCompareNames);
				}
			}
		}
		else if (fieldNode->IsName("static_ubo") || fieldNode->IsName("ubo"))
		{
			string uboName = fieldNode->GetString();
			bool dynamic = fieldNode->IsName("static_ubo") ? false : true;
			auto ubo = ShaderManager::Instance().FindUBO(uboName);
			Assert(ubo != nullptr, STR("Cannot find ubo {} referenced by material {}", uboName, name));

			auto mbo = new MaterialBufferObject;
			if (dynamic)
				mbo->uboInstance = ubo->dynamicInstance;
			else
			{
				mbo->uboInstance = new UBOInfoInstance;
				mbo->uboInstance->isDynamic = false;
				mbo->uboInstance->ubo = ubo;
			}
			buffers.push_back(mbo);

			auto uniformNodes = fieldNode->GetChildren();
			for (auto uniformNode : uniformNodes)
			{
				string uniformName = uniformNode->GetName();

				// find the field in our ubo
				auto member = FindUniformMember(ubo, uniformName);
				Assert(member != nullptr, STR("Material uniform {} does not exist in ubo {}", uniformName, uboName));
				switch (member->type)
				{
					case UniformType_vec4:
					{
						auto uniform = new MaterialUniform_vec4(uniformNode->GetString());
						uniform->value = uniformNode->GetVector4();
						mbo->uniforms.push_back(uniform);
					}
					break;

					case UniformType_ivec4:
					{
						auto uniform = new MaterialUniform_ivec4(uniformNode->GetString());
						uniform->value = uniformNode->GetVector4i();
						mbo->uniforms.push_back(uniform);
					}
					break;

					case UniformType_mat4x4:
					{
						auto uniform = new MaterialUniform_mat4x4(uniformNode->GetString());
						vec3 rot = uniformNode->GetVector3();
						vec3 scale = uniformNode->GetVector3();
						vec3 pos = uniformNode->GetVector3();
						uniform->value = MakeMatrix(rot, scale, pos);
						mbo->uniforms.push_back(uniform);
					}
					break;

					case UniformType_f32:
					{
						auto uniform = new MaterialUniform_f32(uniformNode->GetString());
						uniform->value = uniformNode->GetF32();
						mbo->uniforms.push_back(uniform);
					}
					break;

					case UniformType_i32:
					{
						auto uniform = new MaterialUniform_i32(uniformNode->GetString());
						uniform->value = uniformNode->GetI32();
						mbo->uniforms.push_back(uniform);
					}
					break;
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
	stream.WriteString(shaderName);

	stream.WriteU8((u8)buffers.size());
	for (auto mbo : buffers)
	{
		stream.WriteString(mbo->uboInstance->ubo->structName);
		stream.WriteU8((u8)mbo->uniforms.size());
		for (auto uniform : mbo->uniforms)
		{
			stream.WriteString(uniform->uniformName);
			stream.WriteU8((u8)uniform->type);
			switch (uniform->type)
			{
				case UniformType_vec4:
				{
					auto u = (MaterialUniform_vec4*)uniform;
					stream.WriteF32(u->value.x);
					stream.WriteF32(u->value.y);
					stream.WriteF32(u->value.z);
					stream.WriteF32(u->value.w);
				}
				break;
				case UniformType_ivec4:
				{
					auto u = (MaterialUniform_ivec4*)uniform;
					stream.WriteI32(u->value.x);
					stream.WriteI32(u->value.y);
					stream.WriteI32(u->value.z);
					stream.WriteI32(u->value.w);
				}
				break;
				case UniformType_mat4x4:
				{
					auto u = (MaterialUniform_mat4x4*)uniform;
					for (int r = 0; r < 4; r++)
					{
						for (int c = 0; c < 4; c++)
						{
							stream.WriteF32(u->value[r][c]);
						}
					}
				}
				break;
				case UniformType_f32:
				{
					auto u = (MaterialUniform_f32*)uniform;
					stream.WriteF32(u->value);
				}
				break;
				case UniformType_i32:
				{
					auto u = (MaterialUniform_i32*)uniform;
					stream.WriteI32(u->value);
				}
				break;
			}
		}
	}

	stream.WriteU8((u8)samplers.size());
	for (auto sampler : samplers)
	{
		stream.WriteString(sampler->samplerName);
		stream.WriteString(sampler->textureName);
		stream.WriteU8((u8)sampler->minFilter);
		stream.WriteU8((u8)sampler->magFilter);
		stream.WriteU8((u8)sampler->uWrap);
		stream.WriteU8((u8)sampler->vWrap);
		stream.WriteU8((u8)sampler->compare);
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
	shaderName = stream.ReadString();

	size_t bufferCount = stream.ReadU8();
	for (size_t i=0; i< bufferCount; i++)
	{
		string structName = stream.ReadString();

		auto mbo = new MaterialBufferObject;
		buffers.push_back(mbo);
		mbo->uboInstance = new UBOInfoInstance;
		mbo->uboInstance->isDynamic = false;
		mbo->uboInstance->ubo = ShaderManager::Instance().FindUBO(structName);
		Assert(mbo->uboInstance->ubo, STR("Cannot find UBO {}", structName));

		size_t uniformCount = stream.ReadU8();
		for (size_t i=0; i<uniformCount; i++)
		{
			string uniformName = stream.ReadString();
			UniformType uniformType = (UniformType)stream.ReadU8();

			switch (uniformType)
			{
				case UniformType_vec4:
				{
					auto uniform = new MaterialUniform_vec4(uniformName);
					uniform->value.x = stream.ReadF32();
					uniform->value.y = stream.ReadF32();
					uniform->value.z = stream.ReadF32();
					uniform->value.w = stream.ReadF32();
				}
				break;
				case UniformType_ivec4:
				{
					auto uniform = new MaterialUniform_ivec4(uniformName);
					uniform->value.x = stream.ReadI32();
					uniform->value.y = stream.ReadI32();
					uniform->value.z = stream.ReadI32();
					uniform->value.w = stream.ReadI32();
				}
				break;
				case UniformType_mat4x4:
				{
					auto uniform = new MaterialUniform_mat4x4(uniformName);
					for (int r = 0; r < 4; r++)
					{
						for (int c = 0; c < 4; c++)
						{
							uniform->value[r][c] = stream.ReadF32();
						}
					}
				}
				break;
				case UniformType_f32:
				{
					auto uniform = new MaterialUniform_f32(uniformName);
					uniform->value = stream.ReadF32();
				}
				break;
				case UniformType_i32:
				{
					auto uniform = new MaterialUniform_i32(uniformName);
					uniform->value = stream.ReadI32();
				}
				break;
			}
		}
	}

	size_t sampleCount = stream.ReadU8();
	for (size_t i=0; i<sampleCount; i++)
	{
		auto sampler = new MaterialSampler;
		sampler->samplerName = stream.ReadString();
		sampler->textureName = stream.ReadString();
		sampler->minFilter = (SamplerFilter)stream.ReadU8();
		sampler->magFilter = (SamplerFilter)stream.ReadU8();
		sampler->uWrap = (SamplerWrap)stream.ReadU8();
		sampler->vWrap = (SamplerWrap)stream.ReadU8();
		sampler->compare = (SamplerCompare)stream.ReadU8();
		samplers.push_back(sampler);
	}

	return true;
}


