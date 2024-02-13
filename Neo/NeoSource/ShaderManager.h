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
	UBOMemberType type;	// mat4x4, vec4, ivec4
	u32 offset;
	u32 members;
};

struct UBOInfo
{
	string name;
	string structName;
	u32 size;
	vector<UBOMemberInfo> members;
};

class ShaderManager : public Module<ShaderManager>
{
	hashtable<string, UBOInfo> m_ubos;

public:
	ShaderManager();
	~ShaderManager();

	void RegisterUBO(const UBOInfo &uboInfo);
	string UBOContentsToString(const UBOInfo& uboInfo);

	UBOInfo* FindUBO(const string& name)
	{
		auto it = m_ubos.find(name);
		return (it != m_ubos.end()) ? &it->second : nullptr;
	}
};
