#pragma once

enum VertexFormat
{
	Fmt_R8G8B8A8_UNORM,
	Fmt_R32G32_SFLOAT,
	Fmt_R32G32B32_SFLOAT
};

struct Vertex_p3f_t2f_c4b
{
	vec3 pos;
	vec2 texCoord;
	u32 color;

	bool operator==(const Vertex_p3f_t2f_c4b& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};
template<> struct std::hash<Vertex_p3f_t2f_c4b>
{
	size_t operator()(Vertex_p3f_t2f_c4b const& vertex) const
	{
		return ((std::hash<glm::vec3>()(vertex.pos) ^ (std::hash<u32>()(vertex.color) << 1)) >> 1) ^ (std::hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};


enum VertAttribType
{
	VertAttribType_f32,
	VertAttribType_i32,
	VertAttribType_vec2,
	VertAttribType_vec3,
	VertAttribType_vec4,
	VertAttribType_ivec2,
	VertAttribType_ivec3,
	VertAttribType_ivec4
};
extern hashtable<string, VertAttribType> VertAttribType_Lookup;
extern hashtable<VertAttribType, string> VertAttribTypeToString_Lookup;

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

struct UBO_RenderPass
{
	i32 viewIndex;
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
	u32 datasize = sizeof(vec4);
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

struct InputAttributeInfo
{
	string name;
	int binding;
	int location;
	VertexFormat format;
	int offset;
};

struct InputAttributesBindingInfo
{
	int binding;
	int stride;
	bool bindingIsPerInstance;		// per vertex of per instance?
};

struct InputAttributesDescription
{
	string name;
	vector<InputAttributeInfo> attributes;
	vector<InputAttributesBindingInfo> bindings;
	struct IADPlatformData* platformData;
};

struct RenderPassInfo
{
	vector<TextureRef> renderTargets;
	struct RPIPlatformData* platformData;
};


class ShaderManager : public Module<ShaderManager>
{
	hashtable<string, UBOInfo*> m_ubos;
	hashtable<string, InputAttributesDescription*> m_iads;

public:
	ShaderManager();
	~ShaderManager();

	void RegisterUBO(UBOInfo *uboInfo);
	string UBOContentsToString(const UBOInfo &uboInfo);
	UBOInfoInstance* CreateUBOInstance(UBOInfo *uboInfo, bool dynamic);
	UBOInfo* FindUBO(const string& name)
	{
		auto it = m_ubos.find(name);
		return (it != m_ubos.end()) ? it->second : nullptr;
	}

	void RegisterIAD(InputAttributesDescription* iad);
	void CreatePlatformData();
	InputAttributesDescription* FindIAD(const string& name)
	{
		auto it = m_iads.find(name);
		return (it != m_iads.end()) ? it->second : nullptr;
	}
};

