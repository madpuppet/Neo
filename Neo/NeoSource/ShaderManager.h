#pragma once

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

enum UBOMemberType
{
	UBOMemberType_Matrix,
	UBOMemberType_Vector,
	UBOMemberType_IVector
};

struct UBOMemberInfo
{
	string name;
	UBOMemberType type = UBOMemberType_Vector;	// mat4x4, vec4, ivec4
	u32 offset = 0;
	u32 members = 1;
};

struct UBOInfo
{
	string name;
	string structName;
	bool isDynamic = false;
	u32 size = 0;
	vector<UBOMemberInfo> members;

	UniformBufferPlatformData* platformData = nullptr;
};

class ShaderManager : public Module<ShaderManager>
{
	hashtable<string, UBOInfo> m_ubos;

public:
	ShaderManager();
	~ShaderManager();

	void RegisterUBO(const UBOInfo &uboInfo);
	void CreateUBOPlatformData();
	string UBOContentsToString(const UBOInfo& uboInfo);

	UBOInfo* FindUBO(const string& name)
	{
		auto it = m_ubos.find(name);
		return (it != m_ubos.end()) ? &it->second : nullptr;
	}
};
