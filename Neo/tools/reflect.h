#pragma once
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
	GenericCallback func;
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
	hashtable<string,int> map_stringToInt;
	hashtable<int,string> map_intToString;
};
extern void NeoReflectionInit();
//#### ENUM TextureLayout ####
extern ReflectEnumInfo enumReflect_TextureLayout;
TextureLayout TextureLayout_StringToEnum(string name)
{
  auto it = enumReflect_TextureLayout.map_stringToInt.find(name);
  if (it != enumReflect_TextureLayout.map_stringToInt.end())
    return it->second
  return ;
}
string TextureLayout_EnumToString(TextureLayout value)
{
  auto it = enumReflect_TextureLayout.map_intToString.find((int)value);
  if (it != enumReflect_TextureLayout.map_intToString.end())
    return (TextureLayout)it->second
  return (TextureLayout)-1;
}

//#### ENUM TextureType ####
extern ReflectEnumInfo enumReflect_TextureType;
TextureType TextureType_StringToEnum(string name)
{
  auto it = enumReflect_TextureType.map_stringToInt.find(name);
  if (it != enumReflect_TextureType.map_stringToInt.end())
    return it->second
  return ;
}
string TextureType_EnumToString(TextureType value)
{
  auto it = enumReflect_TextureType.map_intToString.find((int)value);
  if (it != enumReflect_TextureType.map_intToString.end())
    return (TextureType)it->second
  return (TextureType)-1;
}

//#### ENUM TexturePixelFormat ####
extern ReflectEnumInfo enumReflect_TexturePixelFormat;
TexturePixelFormat TexturePixelFormat_StringToEnum(string name)
{
  auto it = enumReflect_TexturePixelFormat.map_stringToInt.find(name);
  if (it != enumReflect_TexturePixelFormat.map_stringToInt.end())
    return it->second
  return ;
}
string TexturePixelFormat_EnumToString(TexturePixelFormat value)
{
  auto it = enumReflect_TexturePixelFormat.map_intToString.find((int)value);
  if (it != enumReflect_TexturePixelFormat.map_intToString.end())
    return (TexturePixelFormat)it->second
  return (TexturePixelFormat)-1;
}

//#### STRUCT TestStruct ####
extern ReflectStructInfo reflectStructInfo_TestStruct;
//#### ENUM ApplicationThreads ####
extern ReflectEnumInfo enumReflect_ApplicationThreads;
ApplicationThreads ApplicationThreads_StringToEnum(string name)
{
  auto it = enumReflect_ApplicationThreads.map_stringToInt.find(name);
  if (it != enumReflect_ApplicationThreads.map_stringToInt.end())
    return it->second
  return ;
}
string ApplicationThreads_EnumToString(ApplicationThreads value)
{
  auto it = enumReflect_ApplicationThreads.map_intToString.find((int)value);
  if (it != enumReflect_ApplicationThreads.map_intToString.end())
    return (ApplicationThreads)it->second
  return (ApplicationThreads)-1;
}

