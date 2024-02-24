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
	UBOMemberInfo* uboMember;
	void* data;
};

struct MaterialBufferObject
{
	vector<MaterialUniform> uniforms;
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

	// dirty mask gets set if any uniforms are updated (bit 0 is frame 0, bit 1 is frame 1)
	u32 dirtyMask = 0xf;

	void SetUniform(const string& name, UniformType type, const void* data, bool flush);

public:
	Material(const string& name);
	Material(const string& name, Material *parent);
	virtual ~Material();

	void SetUniform_vec4(const string& name, const vec4& value, bool flush) { SetUniform(name, UniformType_vec4, &value, flush); }
	void SetUniform_ivec4(const string& name, const ivec4& value, bool flush) { SetUniform(name, UniformType_ivec4, &value, flush); }
	void SetUniform_f32(const string& name, f32 value, bool flush) { SetUniform(name, UniformType_f32, &value, flush); }
	void SetUniform_i32(const string& name, i32 value, bool flush) { SetUniform(name, UniformType_ivec4, &value, flush); }
	void SetUniform_mat4x4(const string& name, const mat4x4& value, bool flush) { SetUniform(name, UniformType_mat4x4, &value, flush); }

	MaterialAssetData* GetAssetData() { return m_assetData; }
	MaterialPlatformData* GetPlatformData() { return m_platformData; }
};

// texture factory keeps a map of all the currently created textures
class MaterialFactory : public Module<MaterialFactory>
{
	hashtable<u64, Material*> m_resources;
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

