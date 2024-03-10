#pragma once
void NeoInitReflection();

enum MemoryGroup;
bool MemoryGroup_StringToEnum(string name, MemoryGroup &value);
bool MemoryGroup_EnumToString(MemoryGroup value, string &name);

enum SerializerError;
bool SerializerError_StringToEnum(string name, SerializerError &value);
bool SerializerError_EnumToString(SerializerError value, string &name);

enum NeoModuleInitPri;
bool NeoModuleInitPri_StringToEnum(string name, NeoModuleInitPri &value);
bool NeoModuleInitPri_EnumToString(NeoModuleInitPri value, string &name);

enum NeoModulePri;
bool NeoModulePri_StringToEnum(string name, NeoModulePri &value);
bool NeoModulePri_EnumToString(NeoModulePri value, string &name);

enum VertexFormat;
bool VertexFormat_StringToEnum(string name, VertexFormat &value);
bool VertexFormat_EnumToString(VertexFormat value, string &name);

enum VertAttribType;
bool VertAttribType_StringToEnum(string name, VertAttribType &value);
bool VertAttribType_EnumToString(VertAttribType value, string &name);

enum TextureLayout;
bool TextureLayout_StringToEnum(string name, TextureLayout &value);
bool TextureLayout_EnumToString(TextureLayout value, string &name);

enum TextureType;
bool TextureType_StringToEnum(string name, TextureType &value);
bool TextureType_EnumToString(TextureType value, string &name);

enum TexturePixelFormat;
bool TexturePixelFormat_StringToEnum(string name, TexturePixelFormat &value);
bool TexturePixelFormat_EnumToString(TexturePixelFormat value, string &name);

//#### STRUCT TestStruct ####
extern ReflectStructInfo reflectStructInfo_TestStruct;
enum ThreadGUID;
bool ThreadGUID_StringToEnum(string name, ThreadGUID &value);
bool ThreadGUID_EnumToString(ThreadGUID value, string &name);

enum SROType;
bool SROType_StringToEnum(string name, SROType &value);
bool SROType_EnumToString(SROType value, string &name);

enum SROStage;
bool SROStage_StringToEnum(string name, SROStage &value);
bool SROStage_EnumToString(SROStage value, string &name);

enum Alignment;
bool Alignment_StringToEnum(string name, Alignment &value);
bool Alignment_EnumToString(Alignment value, string &name);

enum MaterialBlendMode;
bool MaterialBlendMode_StringToEnum(string name, MaterialBlendMode &value);
bool MaterialBlendMode_EnumToString(MaterialBlendMode value, string &name);

enum MaterialCullMode;
bool MaterialCullMode_StringToEnum(string name, MaterialCullMode &value);
bool MaterialCullMode_EnumToString(MaterialCullMode value, string &name);

enum SamplerFilter;
bool SamplerFilter_StringToEnum(string name, SamplerFilter &value);
bool SamplerFilter_EnumToString(SamplerFilter value, string &name);

enum SamplerWrap;
bool SamplerWrap_StringToEnum(string name, SamplerWrap &value);
bool SamplerWrap_EnumToString(SamplerWrap value, string &name);

enum SamplerCompare;
bool SamplerCompare_StringToEnum(string name, SamplerCompare &value);
bool SamplerCompare_EnumToString(SamplerCompare value, string &name);

enum VarType;
bool VarType_StringToEnum(string name, VarType &value);
bool VarType_EnumToString(VarType value, string &name);

