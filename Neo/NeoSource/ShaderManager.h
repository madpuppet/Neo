#pragma once

enum UniformType
{
	UniformType_vec4,
	UniformType_ivec4,
	UniformType_mat4x4,
	UniformType_f32,
	UniformType_i32
};
extern const char* UniformTypeToString[];

// common View ubo
struct UBO_View
{
	mat4x4 view;
	mat4x4 proj;
	mat4x4 ortho;
};

struct UBO_Material
{
	vec4 blend;
};

struct UBO_Model
{
	mat4x4 model;
};

struct UBOMemberInfo
{
	string name;
	UniformType type = UniformType_vec4;
	u32 offset = 0;
	u32 members = 1;
};

struct UBOInfo
{
	string structName;
	u32 size = 0;
	vector<UBOMemberInfo> members;

	// we automatically create a dynamic instance for each UBO
	// users should create their own instance if they want a static instance
	struct UBOInfoInstance* dynamicInstance;
};

struct UBOInfoInstance
{
	bool isDynamic = false;
	UBOInfo* ubo = nullptr;
	UniformBufferPlatformData* platformData = nullptr;
};

class ShaderManager : public Module<ShaderManager>
{
	hashtable<string, UBOInfo*> m_ubos;

public:
	ShaderManager();
	~ShaderManager();

	void RegisterUBO(UBOInfo *uboInfo);
	void CreateInstances();
	string UBOContentsToString(const UBOInfo &uboInfo);
	UBOInfoInstance* CreateUBOInstance(UBOInfo *uboInfo, bool dynamic);

	UBOInfo* FindUBO(const string& name)
	{
		auto it = m_ubos.find(name);
		return (it != m_ubos.end()) ? it->second : nullptr;
	}
};
