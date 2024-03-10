#include "Neo.h"
#include "ShaderManager.h"
#include "RenderThread.h"

DECLARE_MODULE(ShaderManager, NeoModuleInitPri_ShaderManager, NeoModulePri_None);

ShaderManager::ShaderManager() 
{
	UBOMemberInfo mi_view{ "view", VarType_mat4x4, offsetof(UBO_View, view), 1, sizeof(mat4x4) };
	UBOMemberInfo mi_proj{ "proj", VarType_mat4x4, offsetof(UBO_View, proj), 1, sizeof(mat4x4) };
	UBOMemberInfo mi_ortho{ "ortho", VarType_mat4x4, offsetof(UBO_View, ortho), 1, sizeof(mat4x4) };
	auto i_View = new UBOInfo{ "UBO_View", (u32)sizeof(UBO_View), {mi_view,mi_proj,mi_ortho}};
	RegisterUBO(i_View);

	UBOMemberInfo mi_blend{ "blendColor", VarType_vec4, offsetof(UBO_Material, blend), 1, sizeof(vec4) };
	auto i_Material = new UBOInfo{ "UBO_Material", (u32)sizeof(UBO_Material), {mi_blend} };
	RegisterUBO(i_Material);

	UBOMemberInfo mi_model{ "model", VarType_mat4x4, offsetof(UBO_Model, model), 1, sizeof(mat4x4) };
	auto i_Model = new UBOInfo{ "UBO_Model", (u32)sizeof(UBO_Model), {mi_model} };
	RegisterUBO(i_Model);

	InputAttributesBindingInfo attribBinding_p3f_t2f_c4b{ 0, sizeof(Vertex_p3f_t2f_c4b), false };
	InputAttributeInfo attrib_pos3f{ "pos", 0, 0, Fmt_R32G32B32_SFLOAT, offsetof(Vertex_p3f_t2f_c4b,pos) };
	InputAttributeInfo attrib_texCoord2f{ "texCoord", 0, 1, Fmt_R32G32_SFLOAT, offsetof(Vertex_p3f_t2f_c4b,texCoord) };
	InputAttributeInfo attrib_color4b{ "color", 0, 2, Fmt_R8G8B8A8_UNORM, offsetof(Vertex_p3f_t2f_c4b,color) };
	auto vertex_p3f_t2f_c4b = new InputAttributesDescription{ "Vertex_p3f_t2f_c4b", {attrib_pos3f, attrib_texCoord2f, attrib_color4b}, {attribBinding_p3f_t2f_c4b} };
	RegisterIAD(vertex_p3f_t2f_c4b);
}

ShaderManager::~ShaderManager(){}

void ShaderManager::RegisterUBO(UBOInfo *uboInfo)
{
	m_ubos[uboInfo->structName] = uboInfo;
}

string ShaderManager::UBOContentsToString(const UBOInfo &uboInfo)
{
	string outStr;
	for (auto member : uboInfo.members)
	{
		if (member.members > 1)
		{
			outStr += std::format("\t{} {}[{}];\n", VarType_EnumToString(member.type), member.name, member.members);
		}
		else
		{
			outStr += std::format("\t{} {};\n", VarType_EnumToString(member.type), member.name);
		}
	}
	return outStr;
}

UBOInfoInstance* ShaderManager::CreateUBOInstance(UBOInfo *uboInfo, bool dynamic)
{
	Assert(Thread::IsOnThread(ThreadGUID_Render), "Must be run on render thread!");

	auto instance = new UBOInfoInstance;
	instance->isDynamic = dynamic;
	instance->ubo = uboInfo;
	instance->platformData = UniformBufferPlatformData_Create(*uboInfo, dynamic);
	return instance;
}

void ShaderManager::RegisterIAD(InputAttributesDescription* iad)
{
	m_iads[iad->name] = iad;
}

void ShaderManager::CreatePlatformData()
{
	for (auto it : m_ubos)
	{
		auto ubo = it.second;
		ubo->dynamicInstance = CreateUBOInstance(ubo, true);
	}

	for (auto it : m_iads)
	{
		auto iad = it.second;
		iad->platformData = IADPlatformData_Create(iad);
	}
}
