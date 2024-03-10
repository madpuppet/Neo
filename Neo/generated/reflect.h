#pragma once
void NeoInitReflection();

enum FSExcludeType;
bool FSExcludeType_StringToEnum(string name, FSExcludeType &value);
string FSExcludeType_EnumToString(FSExcludeType value);

enum MemoryGroup;
bool MemoryGroup_StringToEnum(string name, MemoryGroup &value);
string MemoryGroup_EnumToString(MemoryGroup value);

enum SerializerError;
bool SerializerError_StringToEnum(string name, SerializerError &value);
string SerializerError_EnumToString(SerializerError value);

enum NeoModuleInitPri;
bool NeoModuleInitPri_StringToEnum(string name, NeoModuleInitPri &value);
string NeoModuleInitPri_EnumToString(NeoModuleInitPri value);

enum NeoModulePri;
bool NeoModulePri_StringToEnum(string name, NeoModulePri &value);
string NeoModulePri_EnumToString(NeoModulePri value);

enum SROType;
bool SROType_StringToEnum(string name, SROType &value);
string SROType_EnumToString(SROType value);

enum SROStage;
bool SROStage_StringToEnum(string name, SROStage &value);
string SROStage_EnumToString(SROStage value);

enum TextureLayout;
bool TextureLayout_StringToEnum(string name, TextureLayout &value);
string TextureLayout_EnumToString(TextureLayout value);

enum TextureType;
bool TextureType_StringToEnum(string name, TextureType &value);
string TextureType_EnumToString(TextureType value);

enum TexturePixelFormat;
bool TexturePixelFormat_StringToEnum(string name, TexturePixelFormat &value);
string TexturePixelFormat_EnumToString(TexturePixelFormat value);

//#### STRUCT TestStruct ####
extern ReflectStructInfo reflectStructInfo_TestStruct;
enum ThreadGUID;
bool ThreadGUID_StringToEnum(string name, ThreadGUID &value);
string ThreadGUID_EnumToString(ThreadGUID value);

enum VertexFormat;
bool VertexFormat_StringToEnum(string name, VertexFormat &value);
string VertexFormat_EnumToString(VertexFormat value);

enum VertAttribType;
bool VertAttribType_StringToEnum(string name, VertAttribType &value);
string VertAttribType_EnumToString(VertAttribType value);

enum Alignment;
bool Alignment_StringToEnum(string name, Alignment &value);
string Alignment_EnumToString(Alignment value);

enum MaterialBlendMode;
bool MaterialBlendMode_StringToEnum(string name, MaterialBlendMode &value);
string MaterialBlendMode_EnumToString(MaterialBlendMode value);

enum MaterialCullMode;
bool MaterialCullMode_StringToEnum(string name, MaterialCullMode &value);
string MaterialCullMode_EnumToString(MaterialCullMode value);

enum SamplerFilter;
bool SamplerFilter_StringToEnum(string name, SamplerFilter &value);
string SamplerFilter_EnumToString(SamplerFilter value);

enum SamplerWrap;
bool SamplerWrap_StringToEnum(string name, SamplerWrap &value);
string SamplerWrap_EnumToString(SamplerWrap value);

enum SamplerCompare;
bool SamplerCompare_StringToEnum(string name, SamplerCompare &value);
string SamplerCompare_EnumToString(SamplerCompare value);

enum VarType;
bool VarType_StringToEnum(string name, VarType &value);
string VarType_EnumToString(VarType value);

