#include "Neo.h"
#include "Reflect.h"
#include "Texture.h"
ReflectStructInfo reflectStructInfo_TestStruct
{
  "TestStruct", sizeof(TestStruct),
  {,
    { "pos", VarType_vec3, sizeof(vec3), offset(TestStruct, pos) },
    { "time", VarType_f32, sizeof(f32), offset(TestStruct, time) },
    { "OnStart", VarType_func, 0, 0, [](void* obj) { ((TestStruct*)obj)->OnStart(); } },
    { "OnEnd", VarType_func, 0, 0, [](void* obj) { ((TestStruct*)obj)->OnEnd(); } },
    { "OnDoNothing", VarType_func, 0, 0, [](void* obj) { ((TestStruct*)obj)->OnDoNothing(); } }
  }
};
#include "Application.h"
void NeoReflectionInit()
{
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
  enumReflect_ApplicationThreads.name="ApplicationThreads";
  enumReflect_ApplicationThreads.map_intToString.insert({ ThreadGUID_MAX + 0,"UpdateWorkerThread" });
  enumReflect_ApplicationThreads.map_stringToInt.insert({ "UpdateWorkerThread",ThreadGUID_MAX + 0 });
}
