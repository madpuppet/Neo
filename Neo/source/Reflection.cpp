#include "Neo.h"
#include "Reflection.h"

DECLARE_MODULE(Reflection, NeoModuleInitPri_Reflect, NeoModulePri_None)

#define MAX_ENUMS 256
#define MAX_STRUCTS 256

ReflectEnumInfo* s_enums[MAX_ENUMS];
int s_enums_count = 0;

ReflectStructInfo* s_structs[MAX_STRUCTS];
int s_structs_count = 0;

void NeoRegisterReflectionEnum(ReflectEnumInfo * info)
{
	s_enums[s_enums_count++] = info;
}

void NeoRegisterReflectionStruct(ReflectStructInfo* info)
{
	s_structs[s_structs_count++] = info;
}

void Reflection::Startup()
{
	for (int i = 0; i < s_enums_count; i++)
	{
		m_reflectEnums[s_enums[i]->name] = s_enums[i];
	}
	for (int i = 0; i < s_structs_count; i++)
	{
		m_reflectStructs[s_structs[i]->name] = s_structs[i];
	}
}


