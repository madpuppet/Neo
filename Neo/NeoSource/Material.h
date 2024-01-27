#pragma once

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"
#include "Shader.h"

enum MaterialBlendMode
{
	MaterialBlendMode_Opaque,			// use src color, no blending
	MaterialBlendMode_Alpha,			// pre-multiplied alpha (one,inv_alpha)
	MaterialBlendMode_Additive,			// add src color to destination
	MaterialBlendMode_Subtractive,		// subtract src color from destination
	MaterialBlendMode_Darken,			// darken by the src alpha
	MaterialBlendMode_Fade				// fade dest color by src alpha
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

enum UniformType
{
	UniformType_Texture,
	UniformType_Vec4,
	UniformType_F32,
	UniformType_I32
};

struct MaterialUniform
{
	MaterialUniform(const string& _uniformName, UniformType _type) : uniformName(_uniformName), type(_type) {}
	virtual ~MaterialUniform() {}

	string uniformName;
	UniformType type;
};

struct MaterialUniform_Texture : public MaterialUniform
{
	MaterialUniform_Texture(const string& _uniformName) : MaterialUniform(_uniformName, UniformType_Texture) {}
	virtual ~MaterialUniform_Texture() {}

	string textureName;
	SamplerFilter minFilter = SamplerFilter_Linear;
	SamplerFilter magFilter = SamplerFilter_LinearMipLinear;
	SamplerWrap uWrap = SamplerWrap_Repeat;
	SamplerWrap vWrap = SamplerWrap_Repeat;
	SamplerCompare compare = SamplerCompare_None;

	TextureRef texture;
};
struct MaterialUniform_Vec4 : public MaterialUniform
{
	MaterialUniform_Vec4(const string& _uniformName) : MaterialUniform(_uniformName, UniformType_Vec4) {}

	vec4 value = { 1,1,1,1 };
};
struct MaterialUniform_F32 : public MaterialUniform
{
	MaterialUniform_F32(const string& _uniformName) : MaterialUniform(_uniformName, UniformType_F32) {}

	f32 value = 0.0f;
};
struct MaterialUniform_I32 : public MaterialUniform
{
	MaterialUniform_I32(const string& _uniformName) : MaterialUniform(_uniformName, UniformType_I32) {}

	i32 value = 0;
};

class Material : public Resource
{
	void OnAssetDeliver(struct AssetData* data);
	virtual AssetType GetAssetType() { return AssetType_Material; }

	virtual void Reload() override;

	struct MaterialAssetData* m_assetData;
	struct MaterialPlatformData* m_platformData;

public:
	Material(const string& name);
	Material(const string& name, Material *parent);
	virtual ~Material();
};

// texture factory keeps a map of all the currently created textures
class MaterialFactory : public Module<MaterialFactory>
{
	map<u64, Material*> m_resources;

public:
	MaterialFactory();

	Material* Create(const string& name);
	Material* CreateInstance(const string& name, const string& original);
	void Destroy(Material* texture);
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

	static AssetData* Create(vector<MemBlock> srcFiles, AssetCreateParams* params);
	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;

	string vertexShaderName;
	string pixelShaderName;
	MaterialBlendMode blendMode = MaterialBlendMode_Opaque;
	MaterialCullMode cullMode = MaterialCullMode_None;
	vector<MaterialUniform*> uniforms;
	bool zread = false;
	bool zwrite = false;

	ShaderRef vertexShader;
	ShaderRef pixelShader;

	MaterialRef m_parent;
};

