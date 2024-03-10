#pragma once

#include "Module.h"

//<REFLECT>
enum VarType
{
	VarType_u8,
	VarType_u16,
	VarType_u32,
	VarType_u64,
	VarType_i8,
	VarType_i16,
	VarType_i32,
	VarType_i64,
	VarType_f32,
	VarType_f64,
	VarType_vec2,
	VarType_vec3,
	VarType_vec4,
	VarType_ivec2,
	VarType_ivec3,
	VarType_ivec4,
	VarType_func
};

struct ReflectStructMemberInfo
{
	string name;
	VarType type;
	u32 size;
	u32 offset;
	std::function<void(void*)> func;
};
struct ReflectStructInfo
{
	string name;
	u32 size;
	vector<ReflectStructMemberInfo> members;
};
struct ReflectEnumInfo
{
	string name;
	hashtable<string, int> map_stringToInt;
	hashtable<int, string> map_intToString;
};

#include "../generated/reflect.h"

class Reflection : public Module<Reflection>
{
	hashtable<string, ReflectStructInfo*> m_reflectStructs;
	hashtable<string, ReflectEnumInfo*> m_reflectEnums;

public:
	virtual void Startup();
};
