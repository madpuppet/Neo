#include "Neo.h"
#include "G:\Projects\madpuppet\Neo\Neo\generated\reflect.h"

#include "Memory.h"
ReflectEnumInfo enumReflect_MemoryGroup;
bool MemoryGroup_StringToEnum(string name, MemoryGroup &value)
{
  auto it = enumReflect_MemoryGroup.map_stringToInt.find(name);
  if (it != enumReflect_MemoryGroup.map_stringToInt.end())
  {
    value = (MemoryGroup)it->second;    return true;
  }
  return false;
}
bool MemoryGroup_EnumToString(MemoryGroup value, string &name)
{
  auto it = enumReflect_MemoryGroup.map_intToString.find((int)value);
  if (it != enumReflect_MemoryGroup.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

#include "Serializer.h"
ReflectEnumInfo enumReflect_SerializerError;
bool SerializerError_StringToEnum(string name, SerializerError &value)
{
  auto it = enumReflect_SerializerError.map_stringToInt.find(name);
  if (it != enumReflect_SerializerError.map_stringToInt.end())
  {
    value = (SerializerError)it->second;    return true;
  }
  return false;
}
bool SerializerError_EnumToString(SerializerError value, string &name)
{
  auto it = enumReflect_SerializerError.map_intToString.find((int)value);
  if (it != enumReflect_SerializerError.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

#include "Module.h"
ReflectEnumInfo enumReflect_NeoModuleInitPri;
bool NeoModuleInitPri_StringToEnum(string name, NeoModuleInitPri &value)
{
  auto it = enumReflect_NeoModuleInitPri.map_stringToInt.find(name);
  if (it != enumReflect_NeoModuleInitPri.map_stringToInt.end())
  {
    value = (NeoModuleInitPri)it->second;    return true;
  }
  return false;
}
bool NeoModuleInitPri_EnumToString(NeoModuleInitPri value, string &name)
{
  auto it = enumReflect_NeoModuleInitPri.map_intToString.find((int)value);
  if (it != enumReflect_NeoModuleInitPri.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectEnumInfo enumReflect_NeoModulePri;
bool NeoModulePri_StringToEnum(string name, NeoModulePri &value)
{
  auto it = enumReflect_NeoModulePri.map_stringToInt.find(name);
  if (it != enumReflect_NeoModulePri.map_stringToInt.end())
  {
    value = (NeoModulePri)it->second;    return true;
  }
  return false;
}
bool NeoModulePri_EnumToString(NeoModulePri value, string &name)
{
  auto it = enumReflect_NeoModulePri.map_intToString.find((int)value);
  if (it != enumReflect_NeoModulePri.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

#include "ShaderManager.h"
ReflectEnumInfo enumReflect_VertexFormat;
bool VertexFormat_StringToEnum(string name, VertexFormat &value)
{
  auto it = enumReflect_VertexFormat.map_stringToInt.find(name);
  if (it != enumReflect_VertexFormat.map_stringToInt.end())
  {
    value = (VertexFormat)it->second;    return true;
  }
  return false;
}
bool VertexFormat_EnumToString(VertexFormat value, string &name)
{
  auto it = enumReflect_VertexFormat.map_intToString.find((int)value);
  if (it != enumReflect_VertexFormat.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectEnumInfo enumReflect_VertAttribType;
bool VertAttribType_StringToEnum(string name, VertAttribType &value)
{
  auto it = enumReflect_VertAttribType.map_stringToInt.find(name);
  if (it != enumReflect_VertAttribType.map_stringToInt.end())
  {
    value = (VertAttribType)it->second;    return true;
  }
  return false;
}
bool VertAttribType_EnumToString(VertAttribType value, string &name)
{
  auto it = enumReflect_VertAttribType.map_intToString.find((int)value);
  if (it != enumReflect_VertAttribType.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

#include "Texture.h"
ReflectEnumInfo enumReflect_TextureLayout;
bool TextureLayout_StringToEnum(string name, TextureLayout &value)
{
  auto it = enumReflect_TextureLayout.map_stringToInt.find(name);
  if (it != enumReflect_TextureLayout.map_stringToInt.end())
  {
    value = (TextureLayout)it->second;    return true;
  }
  return false;
}
bool TextureLayout_EnumToString(TextureLayout value, string &name)
{
  auto it = enumReflect_TextureLayout.map_intToString.find((int)value);
  if (it != enumReflect_TextureLayout.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectEnumInfo enumReflect_TextureType;
bool TextureType_StringToEnum(string name, TextureType &value)
{
  auto it = enumReflect_TextureType.map_stringToInt.find(name);
  if (it != enumReflect_TextureType.map_stringToInt.end())
  {
    value = (TextureType)it->second;    return true;
  }
  return false;
}
bool TextureType_EnumToString(TextureType value, string &name)
{
  auto it = enumReflect_TextureType.map_intToString.find((int)value);
  if (it != enumReflect_TextureType.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectEnumInfo enumReflect_TexturePixelFormat;
bool TexturePixelFormat_StringToEnum(string name, TexturePixelFormat &value)
{
  auto it = enumReflect_TexturePixelFormat.map_stringToInt.find(name);
  if (it != enumReflect_TexturePixelFormat.map_stringToInt.end())
  {
    value = (TexturePixelFormat)it->second;    return true;
  }
  return false;
}
bool TexturePixelFormat_EnumToString(TexturePixelFormat value, string &name)
{
  auto it = enumReflect_TexturePixelFormat.map_intToString.find((int)value);
  if (it != enumReflect_TexturePixelFormat.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectStructInfo reflectStructInfo_TestStruct
{
  "TestStruct", sizeof(TestStruct),
  {
    { "pos", VarType_vec3, sizeof(vec3), offsetof(TestStruct, pos) },
    { "time", VarType_f32, sizeof(f32), offsetof(TestStruct, time) },
    { "OnButtonPress", VarType_func, 0, 0, [](void* obj) { ((TestStruct*)obj)->OnButtonPress(); } }
  }
};
#include "Thread.h"
ReflectEnumInfo enumReflect_ThreadGUID;
bool ThreadGUID_StringToEnum(string name, ThreadGUID &value)
{
  auto it = enumReflect_ThreadGUID.map_stringToInt.find(name);
  if (it != enumReflect_ThreadGUID.map_stringToInt.end())
  {
    value = (ThreadGUID)it->second;    return true;
  }
  return false;
}
bool ThreadGUID_EnumToString(ThreadGUID value, string &name)
{
  auto it = enumReflect_ThreadGUID.map_intToString.find((int)value);
  if (it != enumReflect_ThreadGUID.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

#include "Shader.h"
ReflectEnumInfo enumReflect_SROType;
bool SROType_StringToEnum(string name, SROType &value)
{
  auto it = enumReflect_SROType.map_stringToInt.find(name);
  if (it != enumReflect_SROType.map_stringToInt.end())
  {
    value = (SROType)it->second;    return true;
  }
  return false;
}
bool SROType_EnumToString(SROType value, string &name)
{
  auto it = enumReflect_SROType.map_intToString.find((int)value);
  if (it != enumReflect_SROType.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectEnumInfo enumReflect_SROStage;
bool SROStage_StringToEnum(string name, SROStage &value)
{
  auto it = enumReflect_SROStage.map_stringToInt.find(name);
  if (it != enumReflect_SROStage.map_stringToInt.end())
  {
    value = (SROStage)it->second;    return true;
  }
  return false;
}
bool SROStage_EnumToString(SROStage value, string &name)
{
  auto it = enumReflect_SROStage.map_intToString.find((int)value);
  if (it != enumReflect_SROStage.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

#include "BitmapFont.h"
ReflectEnumInfo enumReflect_Alignment;
bool Alignment_StringToEnum(string name, Alignment &value)
{
  auto it = enumReflect_Alignment.map_stringToInt.find(name);
  if (it != enumReflect_Alignment.map_stringToInt.end())
  {
    value = (Alignment)it->second;    return true;
  }
  return false;
}
bool Alignment_EnumToString(Alignment value, string &name)
{
  auto it = enumReflect_Alignment.map_intToString.find((int)value);
  if (it != enumReflect_Alignment.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

#include "Material.h"
ReflectEnumInfo enumReflect_MaterialBlendMode;
bool MaterialBlendMode_StringToEnum(string name, MaterialBlendMode &value)
{
  auto it = enumReflect_MaterialBlendMode.map_stringToInt.find(name);
  if (it != enumReflect_MaterialBlendMode.map_stringToInt.end())
  {
    value = (MaterialBlendMode)it->second;    return true;
  }
  return false;
}
bool MaterialBlendMode_EnumToString(MaterialBlendMode value, string &name)
{
  auto it = enumReflect_MaterialBlendMode.map_intToString.find((int)value);
  if (it != enumReflect_MaterialBlendMode.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectEnumInfo enumReflect_MaterialCullMode;
bool MaterialCullMode_StringToEnum(string name, MaterialCullMode &value)
{
  auto it = enumReflect_MaterialCullMode.map_stringToInt.find(name);
  if (it != enumReflect_MaterialCullMode.map_stringToInt.end())
  {
    value = (MaterialCullMode)it->second;    return true;
  }
  return false;
}
bool MaterialCullMode_EnumToString(MaterialCullMode value, string &name)
{
  auto it = enumReflect_MaterialCullMode.map_intToString.find((int)value);
  if (it != enumReflect_MaterialCullMode.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectEnumInfo enumReflect_SamplerFilter;
bool SamplerFilter_StringToEnum(string name, SamplerFilter &value)
{
  auto it = enumReflect_SamplerFilter.map_stringToInt.find(name);
  if (it != enumReflect_SamplerFilter.map_stringToInt.end())
  {
    value = (SamplerFilter)it->second;    return true;
  }
  return false;
}
bool SamplerFilter_EnumToString(SamplerFilter value, string &name)
{
  auto it = enumReflect_SamplerFilter.map_intToString.find((int)value);
  if (it != enumReflect_SamplerFilter.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectEnumInfo enumReflect_SamplerWrap;
bool SamplerWrap_StringToEnum(string name, SamplerWrap &value)
{
  auto it = enumReflect_SamplerWrap.map_stringToInt.find(name);
  if (it != enumReflect_SamplerWrap.map_stringToInt.end())
  {
    value = (SamplerWrap)it->second;    return true;
  }
  return false;
}
bool SamplerWrap_EnumToString(SamplerWrap value, string &name)
{
  auto it = enumReflect_SamplerWrap.map_intToString.find((int)value);
  if (it != enumReflect_SamplerWrap.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

ReflectEnumInfo enumReflect_SamplerCompare;
bool SamplerCompare_StringToEnum(string name, SamplerCompare &value)
{
  auto it = enumReflect_SamplerCompare.map_stringToInt.find(name);
  if (it != enumReflect_SamplerCompare.map_stringToInt.end())
  {
    value = (SamplerCompare)it->second;    return true;
  }
  return false;
}
bool SamplerCompare_EnumToString(SamplerCompare value, string &name)
{
  auto it = enumReflect_SamplerCompare.map_intToString.find((int)value);
  if (it != enumReflect_SamplerCompare.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

#include "Reflection.h"
ReflectEnumInfo enumReflect_VarType;
bool VarType_StringToEnum(string name, VarType &value)
{
  auto it = enumReflect_VarType.map_stringToInt.find(name);
  if (it != enumReflect_VarType.map_stringToInt.end())
  {
    value = (VarType)it->second;    return true;
  }
  return false;
}
bool VarType_EnumToString(VarType value, string &name)
{
  auto it = enumReflect_VarType.map_intToString.find((int)value);
  if (it != enumReflect_VarType.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

void NeoInitReflection()
{
  enumReflect_MemoryGroup.name="MemoryGroup";
  enumReflect_MemoryGroup.map_intToString.insert({ 0,"General" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "General",0 });
  enumReflect_MemoryGroup.map_intToString.insert({ 1,"System" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "System",1 });
  enumReflect_MemoryGroup.map_intToString.insert({ 2,"Texture" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "Texture",2 });
  enumReflect_MemoryGroup.map_intToString.insert({ 3,"Models" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "Models",3 });
  enumReflect_MemoryGroup.map_intToString.insert({ 4,"Animation" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "Animation",4 });
  enumReflect_MemoryGroup.map_intToString.insert({ 5,"Props" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "Props",5 });
  enumReflect_MemoryGroup.map_intToString.insert({ 6,"AI" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "AI",6 });
  enumReflect_MemoryGroup.map_intToString.insert({ 7,"User1" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "User1",7 });
  enumReflect_MemoryGroup.map_intToString.insert({ 8,"User2" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "User2",8 });
  enumReflect_MemoryGroup.map_intToString.insert({ 9,"User3" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "User3",9 });
  enumReflect_MemoryGroup.map_intToString.insert({ 10,"User4" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "User4",10 });
  enumReflect_MemoryGroup.map_intToString.insert({ 11,"MAX" });
  enumReflect_MemoryGroup.map_stringToInt.insert({ "MAX",11 });
  enumReflect_SerializerError.name="SerializerError";
  enumReflect_SerializerError.map_intToString.insert({ 0,"OK" });
  enumReflect_SerializerError.map_stringToInt.insert({ "OK",0 });
  enumReflect_SerializerError.map_intToString.insert({ 1,"FileNotFound" });
  enumReflect_SerializerError.map_stringToInt.insert({ "FileNotFound",1 });
  enumReflect_SerializerError.map_intToString.insert({ 2,"BadVersion" });
  enumReflect_SerializerError.map_stringToInt.insert({ "BadVersion",2 });
  enumReflect_SerializerError.map_intToString.insert({ 3,"BufferEmpty" });
  enumReflect_SerializerError.map_stringToInt.insert({ "BufferEmpty",3 });
  enumReflect_SerializerError.map_intToString.insert({ 4,"TruncatedBlock" });
  enumReflect_SerializerError.map_stringToInt.insert({ "TruncatedBlock",4 });
  enumReflect_SerializerError.map_intToString.insert({ 5,"BadData" });
  enumReflect_SerializerError.map_stringToInt.insert({ "BadData",5 });
  enumReflect_NeoModuleInitPri.name="NeoModuleInitPri";
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 0,"Reflect" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "Reflect",0 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 1,"FileManager" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "FileManager",1 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 2,"AssetManager" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "AssetManager",2 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 3,"TimeManager" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "TimeManager",3 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 4,"ResourceLoadedManager" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "ResourceLoadedManager",4 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 5,"TextureFactory" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "TextureFactory",5 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 6,"ShaderFactory" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "ShaderFactory",6 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 7,"MaterialFactory" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "MaterialFactory",7 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 8,"StaticMeshFactory" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "StaticMeshFactory",8 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 9,"BitmapFontFactory" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "BitmapFontFactory",9 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 10,"RenderPassFactory" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "RenderPassFactory",10 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 11,"RenderSceneFactory" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "RenderSceneFactory",11 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 12,"GIL" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "GIL",12 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 13,"RenderThread" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "RenderThread",13 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 14,"ShaderManager" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "ShaderManager",14 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 15,"DefDynamicRenderer" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "DefDynamicRenderer",15 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 16,"ImmDynamicRenderer" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "ImmDynamicRenderer",16 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 17,"Profiler" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "Profiler",17 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 18,"View" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "View",18 });
  enumReflect_NeoModuleInitPri.map_intToString.insert({ 1000 + 0,"Application" });
  enumReflect_NeoModuleInitPri.map_stringToInt.insert({ "Application",1000 + 0 });
  enumReflect_NeoModulePri.name="NeoModulePri";
  enumReflect_NeoModulePri.map_intToString.insert({ 0,"None" });
  enumReflect_NeoModulePri.map_stringToInt.insert({ "None",0 });
  enumReflect_NeoModulePri.map_intToString.insert({ 100 + 0,"First" });
  enumReflect_NeoModulePri.map_stringToInt.insert({ "First",100 + 0 });
  enumReflect_NeoModulePri.map_intToString.insert({ 200 + 0,"Early" });
  enumReflect_NeoModulePri.map_stringToInt.insert({ "Early",200 + 0 });
  enumReflect_NeoModulePri.map_intToString.insert({ 300 + 0,"Mid" });
  enumReflect_NeoModulePri.map_stringToInt.insert({ "Mid",300 + 0 });
  enumReflect_NeoModulePri.map_intToString.insert({ 400 + 0,"Late" });
  enumReflect_NeoModulePri.map_stringToInt.insert({ "Late",400 + 0 });
  enumReflect_NeoModulePri.map_intToString.insert({ 500 + 0,"Last" });
  enumReflect_NeoModulePri.map_stringToInt.insert({ "Last",500 + 0 });
  enumReflect_VertexFormat.name="VertexFormat";
  enumReflect_VertexFormat.map_intToString.insert({ 0,"R8G8B8A8_UNORM" });
  enumReflect_VertexFormat.map_stringToInt.insert({ "R8G8B8A8_UNORM",0 });
  enumReflect_VertexFormat.map_intToString.insert({ 1,"R32G32_SFLOAT" });
  enumReflect_VertexFormat.map_stringToInt.insert({ "R32G32_SFLOAT",1 });
  enumReflect_VertexFormat.map_intToString.insert({ 2,"R32G32B32_SFLOAT" });
  enumReflect_VertexFormat.map_stringToInt.insert({ "R32G32B32_SFLOAT",2 });
  enumReflect_VertAttribType.name="VertAttribType";
  enumReflect_VertAttribType.map_intToString.insert({ 0,"f32" });
  enumReflect_VertAttribType.map_stringToInt.insert({ "f32",0 });
  enumReflect_VertAttribType.map_intToString.insert({ 1,"i32" });
  enumReflect_VertAttribType.map_stringToInt.insert({ "i32",1 });
  enumReflect_VertAttribType.map_intToString.insert({ 2,"vec2" });
  enumReflect_VertAttribType.map_stringToInt.insert({ "vec2",2 });
  enumReflect_VertAttribType.map_intToString.insert({ 3,"vec3" });
  enumReflect_VertAttribType.map_stringToInt.insert({ "vec3",3 });
  enumReflect_VertAttribType.map_intToString.insert({ 4,"vec4" });
  enumReflect_VertAttribType.map_stringToInt.insert({ "vec4",4 });
  enumReflect_VertAttribType.map_intToString.insert({ 5,"ivec2" });
  enumReflect_VertAttribType.map_stringToInt.insert({ "ivec2",5 });
  enumReflect_VertAttribType.map_intToString.insert({ 6,"ivec3" });
  enumReflect_VertAttribType.map_stringToInt.insert({ "ivec3",6 });
  enumReflect_VertAttribType.map_intToString.insert({ 7,"ivec4" });
  enumReflect_VertAttribType.map_stringToInt.insert({ "ivec4",7 });
  enumReflect_TextureLayout.name="TextureLayout";
  enumReflect_TextureLayout.map_intToString.insert({ 0,"Undefined" });
  enumReflect_TextureLayout.map_stringToInt.insert({ "Undefined",0 });
  enumReflect_TextureLayout.map_intToString.insert({ 1,"TransferDest" });
  enumReflect_TextureLayout.map_stringToInt.insert({ "TransferDest",1 });
  enumReflect_TextureLayout.map_intToString.insert({ 2,"ColorAttachment" });
  enumReflect_TextureLayout.map_stringToInt.insert({ "ColorAttachment",2 });
  enumReflect_TextureLayout.map_intToString.insert({ 3,"DepthAttachment" });
  enumReflect_TextureLayout.map_stringToInt.insert({ "DepthAttachment",3 });
  enumReflect_TextureLayout.map_intToString.insert({ 4,"ShaderRead" });
  enumReflect_TextureLayout.map_stringToInt.insert({ "ShaderRead",4 });
  enumReflect_TextureLayout.map_intToString.insert({ 5,"Present" });
  enumReflect_TextureLayout.map_stringToInt.insert({ "Present",5 });
  enumReflect_TextureType.name="TextureType";
  enumReflect_TextureType.map_intToString.insert({ 0,"None" });
  enumReflect_TextureType.map_stringToInt.insert({ "None",0 });
  enumReflect_TextureType.map_intToString.insert({ 1,"Image" });
  enumReflect_TextureType.map_stringToInt.insert({ "Image",1 });
  enumReflect_TextureType.map_intToString.insert({ 2,"ColorBuffer" });
  enumReflect_TextureType.map_stringToInt.insert({ "ColorBuffer",2 });
  enumReflect_TextureType.map_intToString.insert({ 3,"DepthBuffer" });
  enumReflect_TextureType.map_stringToInt.insert({ "DepthBuffer",3 });
  enumReflect_TexturePixelFormat.name="TexturePixelFormat";
  enumReflect_TexturePixelFormat.map_intToString.insert({ 0,"Undefined" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "Undefined",0 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 1,"R4G4_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R4G4_UNORM",1 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 2,"R4G4B4A4_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R4G4B4A4_UNORM",2 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 3,"R5G6B5_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R5G6B5_UNORM",3 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 4,"R5G6B5A1_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R5G6B5A1_UNORM",4 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 5,"R8_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8_UNORM",5 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 6,"R8_SNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8_SNORM",6 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 7,"R8_UINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8_UINT",7 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 8,"R8_SINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8_SINT",8 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 9,"R8_SRGB" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8_SRGB",9 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 10,"R8G8_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8G8_UNORM",10 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 11,"R8G8_SNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8G8_SNORM",11 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 12,"R8G8_UINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8G8_UINT",12 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 13,"R8G8_SINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8G8_SINT",13 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 14,"R8G8B8A8_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8G8B8A8_UNORM",14 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 15,"R8G8B8A8_SNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8G8B8A8_SNORM",15 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 16,"R8G8B8A8_UINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8G8B8A8_UINT",16 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 17,"R8G8B8A8_SINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8G8B8A8_SINT",17 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 18,"R8G8B8A8_SRGB" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R8G8B8A8_SRGB",18 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 19,"B8G8R8A8_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "B8G8R8A8_UNORM",19 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 20,"B8G8R8A8_SNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "B8G8R8A8_SNORM",20 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 21,"B8G8R8A8_UINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "B8G8R8A8_UINT",21 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 22,"B8G8R8A8_SINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "B8G8R8A8_SINT",22 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 23,"B8G8R8A8_SRGB" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "B8G8R8A8_SRGB",23 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 24,"R16_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R16_UNORM",24 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 25,"R16_SNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R16_SNORM",25 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 26,"R16_UINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R16_UINT",26 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 27,"R16_SINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R16_SINT",27 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 28,"R16_SFLOAT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "R16_SFLOAT",28 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 29,"B10G11R11_UFLOAT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "B10G11R11_UFLOAT",29 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 30,"BC1_RGB_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "BC1_RGB_UNORM",30 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 31,"BC1_RGB_SRGB" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "BC1_RGB_SRGB",31 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 32,"BC3_RGBA_UNORM" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "BC3_RGBA_UNORM",32 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 33,"BC3_RGBA_SRGB" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "BC3_RGBA_SRGB",33 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 34,"D32_SFLOAT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "D32_SFLOAT",34 });
  enumReflect_TexturePixelFormat.map_intToString.insert({ 35,"D24_UNORM_S8_UINT" });
  enumReflect_TexturePixelFormat.map_stringToInt.insert({ "D24_UNORM_S8_UINT",35 });
  enumReflect_ThreadGUID.name="ThreadGUID";
  enumReflect_ThreadGUID.map_intToString.insert({ 0,"Main" });
  enumReflect_ThreadGUID.map_stringToInt.insert({ "Main",0 });
  enumReflect_ThreadGUID.map_intToString.insert({ 1,"GILTasks" });
  enumReflect_ThreadGUID.map_stringToInt.insert({ "GILTasks",1 });
  enumReflect_ThreadGUID.map_intToString.insert({ 2,"AssetManager" });
  enumReflect_ThreadGUID.map_stringToInt.insert({ "AssetManager",2 });
  enumReflect_ThreadGUID.map_intToString.insert({ 3,"Render" });
  enumReflect_ThreadGUID.map_stringToInt.insert({ "Render",3 });
  enumReflect_ThreadGUID.map_intToString.insert({ 4,"MAX" });
  enumReflect_ThreadGUID.map_stringToInt.insert({ "MAX",4 });
  enumReflect_SROType.name="SROType";
  enumReflect_SROType.map_intToString.insert({ 0,"Unknown=-1" });
  enumReflect_SROType.map_stringToInt.insert({ "Unknown=-1",0 });
  enumReflect_SROType.map_intToString.insert({ 1,"UBO" });
  enumReflect_SROType.map_stringToInt.insert({ "UBO",1 });
  enumReflect_SROType.map_intToString.insert({ 2,"Sampler" });
  enumReflect_SROType.map_stringToInt.insert({ "Sampler",2 });
  enumReflect_SROStage.name="SROStage";
  enumReflect_SROStage.map_intToString.insert({ 1 + 0,"Geometry" });
  enumReflect_SROStage.map_stringToInt.insert({ "Geometry",1 + 0 });
  enumReflect_SROStage.map_intToString.insert({ 2 + 0,"Vertex" });
  enumReflect_SROStage.map_stringToInt.insert({ "Vertex",2 + 0 });
  enumReflect_SROStage.map_intToString.insert({ 4 + 0,"Fragment" });
  enumReflect_SROStage.map_stringToInt.insert({ "Fragment",4 + 0 });
  enumReflect_Alignment.name="Alignment";
  enumReflect_Alignment.map_intToString.insert({ 0,"TopLeft" });
  enumReflect_Alignment.map_stringToInt.insert({ "TopLeft",0 });
  enumReflect_Alignment.map_intToString.insert({ 1,"TopCenter" });
  enumReflect_Alignment.map_stringToInt.insert({ "TopCenter",1 });
  enumReflect_Alignment.map_intToString.insert({ 2,"TopRight" });
  enumReflect_Alignment.map_stringToInt.insert({ "TopRight",2 });
  enumReflect_Alignment.map_intToString.insert({ 3,"CenterLeft" });
  enumReflect_Alignment.map_stringToInt.insert({ "CenterLeft",3 });
  enumReflect_Alignment.map_intToString.insert({ 4,"Center" });
  enumReflect_Alignment.map_stringToInt.insert({ "Center",4 });
  enumReflect_Alignment.map_intToString.insert({ 5,"CenterRight" });
  enumReflect_Alignment.map_stringToInt.insert({ "CenterRight",5 });
  enumReflect_Alignment.map_intToString.insert({ 6,"BottomLeft" });
  enumReflect_Alignment.map_stringToInt.insert({ "BottomLeft",6 });
  enumReflect_Alignment.map_intToString.insert({ 7,"BottomCenter" });
  enumReflect_Alignment.map_stringToInt.insert({ "BottomCenter",7 });
  enumReflect_Alignment.map_intToString.insert({ 8,"BottomRight" });
  enumReflect_Alignment.map_stringToInt.insert({ "BottomRight",8 });
  enumReflect_MaterialBlendMode.name="MaterialBlendMode";
  enumReflect_MaterialBlendMode.map_intToString.insert({ 0,"Opaque" });
  enumReflect_MaterialBlendMode.map_stringToInt.insert({ "Opaque",0 });
  enumReflect_MaterialBlendMode.map_intToString.insert({ 1,"Alpha" });
  enumReflect_MaterialBlendMode.map_stringToInt.insert({ "Alpha",1 });
  enumReflect_MaterialBlendMode.map_intToString.insert({ 2,"Blend" });
  enumReflect_MaterialBlendMode.map_stringToInt.insert({ "Blend",2 });
  enumReflect_MaterialBlendMode.map_intToString.insert({ 3,"Additive" });
  enumReflect_MaterialBlendMode.map_stringToInt.insert({ "Additive",3 });
  enumReflect_MaterialBlendMode.map_intToString.insert({ 4,"Subtractive" });
  enumReflect_MaterialBlendMode.map_stringToInt.insert({ "Subtractive",4 });
  enumReflect_MaterialCullMode.name="MaterialCullMode";
  enumReflect_MaterialCullMode.map_intToString.insert({ 0,"None" });
  enumReflect_MaterialCullMode.map_stringToInt.insert({ "None",0 });
  enumReflect_MaterialCullMode.map_intToString.insert({ 1,"Front" });
  enumReflect_MaterialCullMode.map_stringToInt.insert({ "Front",1 });
  enumReflect_MaterialCullMode.map_intToString.insert({ 2,"Back" });
  enumReflect_MaterialCullMode.map_stringToInt.insert({ "Back",2 });
  enumReflect_SamplerFilter.name="SamplerFilter";
  enumReflect_SamplerFilter.map_intToString.insert({ 0,"Nearest" });
  enumReflect_SamplerFilter.map_stringToInt.insert({ "Nearest",0 });
  enumReflect_SamplerFilter.map_intToString.insert({ 1,"Linear" });
  enumReflect_SamplerFilter.map_stringToInt.insert({ "Linear",1 });
  enumReflect_SamplerFilter.map_intToString.insert({ 2,"NearestMipNearest" });
  enumReflect_SamplerFilter.map_stringToInt.insert({ "NearestMipNearest",2 });
  enumReflect_SamplerFilter.map_intToString.insert({ 3,"NearestMipLinear" });
  enumReflect_SamplerFilter.map_stringToInt.insert({ "NearestMipLinear",3 });
  enumReflect_SamplerFilter.map_intToString.insert({ 4,"LinearMipNearest" });
  enumReflect_SamplerFilter.map_stringToInt.insert({ "LinearMipNearest",4 });
  enumReflect_SamplerFilter.map_intToString.insert({ 5,"LinearMipLinear" });
  enumReflect_SamplerFilter.map_stringToInt.insert({ "LinearMipLinear",5 });
  enumReflect_SamplerWrap.name="SamplerWrap";
  enumReflect_SamplerWrap.map_intToString.insert({ 0,"Clamp" });
  enumReflect_SamplerWrap.map_stringToInt.insert({ "Clamp",0 });
  enumReflect_SamplerWrap.map_intToString.insert({ 1,"Repeat" });
  enumReflect_SamplerWrap.map_stringToInt.insert({ "Repeat",1 });
  enumReflect_SamplerCompare.name="SamplerCompare";
  enumReflect_SamplerCompare.map_intToString.insert({ 0,"None" });
  enumReflect_SamplerCompare.map_stringToInt.insert({ "None",0 });
  enumReflect_SamplerCompare.map_intToString.insert({ 1,"GEqual" });
  enumReflect_SamplerCompare.map_stringToInt.insert({ "GEqual",1 });
  enumReflect_SamplerCompare.map_intToString.insert({ 2,"LEqual" });
  enumReflect_SamplerCompare.map_stringToInt.insert({ "LEqual",2 });
  enumReflect_VarType.name="VarType";
  enumReflect_VarType.map_intToString.insert({ 0,"u8" });
  enumReflect_VarType.map_stringToInt.insert({ "u8",0 });
  enumReflect_VarType.map_intToString.insert({ 1,"u16" });
  enumReflect_VarType.map_stringToInt.insert({ "u16",1 });
  enumReflect_VarType.map_intToString.insert({ 2,"u32" });
  enumReflect_VarType.map_stringToInt.insert({ "u32",2 });
  enumReflect_VarType.map_intToString.insert({ 3,"u64" });
  enumReflect_VarType.map_stringToInt.insert({ "u64",3 });
  enumReflect_VarType.map_intToString.insert({ 4,"i8" });
  enumReflect_VarType.map_stringToInt.insert({ "i8",4 });
  enumReflect_VarType.map_intToString.insert({ 5,"i16" });
  enumReflect_VarType.map_stringToInt.insert({ "i16",5 });
  enumReflect_VarType.map_intToString.insert({ 6,"i32" });
  enumReflect_VarType.map_stringToInt.insert({ "i32",6 });
  enumReflect_VarType.map_intToString.insert({ 7,"i64" });
  enumReflect_VarType.map_stringToInt.insert({ "i64",7 });
  enumReflect_VarType.map_intToString.insert({ 8,"f32" });
  enumReflect_VarType.map_stringToInt.insert({ "f32",8 });
  enumReflect_VarType.map_intToString.insert({ 9,"f64" });
  enumReflect_VarType.map_stringToInt.insert({ "f64",9 });
  enumReflect_VarType.map_intToString.insert({ 10,"vec2" });
  enumReflect_VarType.map_stringToInt.insert({ "vec2",10 });
  enumReflect_VarType.map_intToString.insert({ 11,"vec3" });
  enumReflect_VarType.map_stringToInt.insert({ "vec3",11 });
  enumReflect_VarType.map_intToString.insert({ 12,"vec4" });
  enumReflect_VarType.map_stringToInt.insert({ "vec4",12 });
  enumReflect_VarType.map_intToString.insert({ 13,"ivec2" });
  enumReflect_VarType.map_stringToInt.insert({ "ivec2",13 });
  enumReflect_VarType.map_intToString.insert({ 14,"ivec3" });
  enumReflect_VarType.map_stringToInt.insert({ "ivec3",14 });
  enumReflect_VarType.map_intToString.insert({ 15,"ivec4" });
  enumReflect_VarType.map_stringToInt.insert({ "ivec4",15 });
  enumReflect_VarType.map_intToString.insert({ 16,"func" });
  enumReflect_VarType.map_stringToInt.insert({ "func",16 });
}
