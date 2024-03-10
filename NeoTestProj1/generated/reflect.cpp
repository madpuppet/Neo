#include "Neo.h"
#include "G:\Projects\madpuppet\Neo\NeoTestProj1\generated\reflect.h"

#include "Application.h"
ReflectEnumInfo enumReflect_ApplicationThreads;
bool ApplicationThreads_StringToEnum(string name, ApplicationThreads &value)
{
  auto it = enumReflect_ApplicationThreads.map_stringToInt.find(name);
  if (it != enumReflect_ApplicationThreads.map_stringToInt.end())
  {
    value = (ApplicationThreads)it->second;    return true;
  }
  return false;
}
bool ApplicationThreads_EnumToString(ApplicationThreads value, string &name)
{
  auto it = enumReflect_ApplicationThreads.map_intToString.find((int)value);
  if (it != enumReflect_ApplicationThreads.map_intToString.end())
  {
    name = it->second;
    return true;
  }
  return false;
}

void GameInitReflection()
{
  enumReflect_ApplicationThreads.name="ApplicationThreads";
  enumReflect_ApplicationThreads.map_intToString.insert({ ThreadGUID_MAX + 0,"UpdateWorkerThread" });
  enumReflect_ApplicationThreads.map_stringToInt.insert({ "UpdateWorkerThread",ThreadGUID_MAX + 0 });
}
