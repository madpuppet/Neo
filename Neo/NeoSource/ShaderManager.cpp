#include "Neo.h"
#include "ShaderManager.h"

DECLARE_MODULE(ShaderManager, NeoModuleInitPri_ShaderManager, NeoModulePri_None, NeoModulePri_None);

ShaderManager::ShaderManager() 
{
	UBOMemberInfo mi_view{ "view", UBOMemberType_Matrix, offsetof(UBO_View, view), 1 };
	UBOMemberInfo mi_proj{ "proj", UBOMemberType_Matrix, offsetof(UBO_View, proj), 1 };
	UBOInfo i_View{ "View", "UBO_View", sizeof(UBO_View), {mi_view,mi_proj}};
	RegisterUBO(i_View);

	UBOMemberInfo mi_blend{ "blend", UBOMemberType_Vector, offsetof(UBO_Material, blend), 1 };
	UBOInfo i_Material{ "Material", "UBO_Material", sizeof(UBO_Material), {mi_blend} };
	RegisterUBO(i_Material);

	UBOMemberInfo mi_model{ "model", UBOMemberType_Matrix, offsetof(UBO_Model, model), 1 };
	UBOInfo i_Model{ "Model", "UBO_Model", sizeof(UBO_Model), {mi_model} };
	RegisterUBO(i_Model);
}

ShaderManager::~ShaderManager(){}

void ShaderManager::RegisterUBO(const UBOInfo &uboInfo)
{
	m_ubos[uboInfo.name] = uboInfo;
}

const char *s_TypeToString[] = { "mat4x4", "vec4", "ivec4" };

string ShaderManager::UBOContentsToString(const UBOInfo &uboInfo)
{
	string outStr;
	for (auto member : uboInfo.members)
	{
		if (member.members > 1)
		{
			outStr += std::format("\t{} {}[{}];\n", s_TypeToString[member.type], member.name, member.members);
		}
		else
		{
			outStr += std::format("\t{} {};\n", s_TypeToString[member.type], member.name);
		}
	}
	return outStr;
}


