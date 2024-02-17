#pragma once

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"
#include "Shader.h"
#include "ShaderManager.h"

enum MaterialBlendMode
{
	MaterialBlendMode_Opaque,			// use src color, no blending
	MaterialBlendMode_Alpha,			// pre-multiplied alpha (one,inv_alpha)   (0.5,0.5,0.5,0.5 == white @ 50% alpha)
	MaterialBlendMode_Blend,			// traditional alpha blending             (0.5,0.5,0.5,0.5 = grey @ 50% alpha)
	MaterialBlendMode_Additive,			// add src color to destination
	MaterialBlendMode_Subtractive,		// subtract src color from destination
};

enum MaterialCullMode
{
	MaterialCullMode_None,				// don't cull any triangles
	MaterialCullMode_Front,				// cull front facing triangles
	MaterialCullMode_Back				// cull back facing triangles
};

enum SamplerFilter
{
	SamplerFilter_Nearest,
	SamplerFilter_Linear,
	SamplerFilter_NearestMipNearest,
	SamplerFilter_NearestMipLinear,
	SamplerFilter_LinearMipNearest,
	SamplerFilter_LinearMipLinear,
};

enum SamplerWrap
{
	SamplerWrap_Clamp,
	SamplerWrap_Repeat
};

enum SamplerCompare
{
	SamplerCompare_None,
	SamplerCompare_GEqual,
	SamplerCompare_LEqual
};

struct MaterialUniform
{
	MaterialUniform(const string& _uniformName, UniformType _type) : uniformName(_uniformName), type(_type) {}
	virtual ~MaterialUniform() {}

	string uniformName;
	UniformType type;
};
struct MaterialUniform_vec4 : public MaterialUniform
{
	MaterialUniform_vec4(const string& _uniformName) : MaterialUniform(_uniformName, UniformType_vec4) {}

	vec4 value = { 1,1,1,1 };
};
struct MaterialUniform_ivec4 : public MaterialUniform
{
	MaterialUniform_ivec4(const string& _uniformName) : MaterialUniform(_uniformName, UniformType_ivec4) {}

	ivec4 value = { 0,0,0,0 };
};
struct MaterialUniform_f32 : public MaterialUniform
{
	MaterialUniform_f32(const string& _uniformName) : MaterialUniform(_uniformName, UniformType_f32) {}

	f32 value = 0.0f;
};
struct MaterialUniform_i32 : public MaterialUniform
{
	MaterialUniform_i32(const string& _uniformName) : MaterialUniform(_uniformName, UniformType_i32) {}

	i32 value = 0;
};

struct MaterialUniform_mat4x4 : public MaterialUniform
{
	MaterialUniform_mat4x4(const string& _uniformName) : MaterialUniform(_uniformName, UniformType_mat4x4) {}

	mat4x4 value = mat4x4(1);
};

struct MaterialBufferObject
{
	vector<MaterialUniform*> uniforms;
	struct UBOInfoInstance* uboInstance = nullptr;
};

struct MaterialSampler
{
	string samplerName;
	string textureName;
	SamplerFilter minFilter = SamplerFilter_Linear;
	SamplerFilter magFilter = SamplerFilter_Linear;
	SamplerFilter mipFilter = SamplerFilter_Linear;
	SamplerWrap uWrap = SamplerWrap_Repeat;
	SamplerWrap vWrap = SamplerWrap_Repeat;
	SamplerCompare compare = SamplerCompare_None;
	TextureRef texture;
};

class Material : public Resource
{
	void OnAssetDeliver(struct AssetData* data);
	virtual AssetType GetAssetType() const override { return AssetType_Material; }

	virtual void Reload() override;

	struct MaterialAssetData* m_assetData = nullptr;
	struct MaterialPlatformData* m_platformData = nullptr;

public:
	Material(const string& name);
	Material(const string& name, Material *parent);
	virtual ~Material();

	MaterialAssetData* GetAssetData() { return m_assetData; }
	MaterialPlatformData* GetPlatformData() { return m_platformData; }
};

// texture factory keeps a map of all the currently created textures
class MaterialFactory : public Module<MaterialFactory>
{
	map<u64, Material*> m_resources;
	Material* m_blank;

public:
	MaterialFactory();

	Material* Create(const string& name);
	Material* CreateInstance(const string& name, const string& original);
	void Destroy(Material* texture);
	Material* GetBlank() { return m_blank; }
};

class MaterialRef : public ResourceRef<Material, MaterialFactory>
{
	void CreateInstance(const string& name, const string& original)
	{
		Destroy();
		m_ptr = MaterialFactory::Instance().CreateInstance(name, original);
	}
};

// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct MaterialAssetData : public AssetData
{
public:
	~MaterialAssetData() {}

	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams* params);

	string shaderName;
	MaterialBlendMode blendMode = MaterialBlendMode_Opaque;
	MaterialCullMode cullMode = MaterialCullMode_None;
	vector<MaterialBufferObject*> buffers;
	vector<MaterialSampler*> samplers;
	bool zread = false;
	bool zwrite = false;

	ShaderRef shader;

	MaterialRef m_parent;
};

