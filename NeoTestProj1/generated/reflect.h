#pragma once
void GameInitReflection();

enum ApplicationThreads;
bool ApplicationThreads_StringToEnum(string name, ApplicationThreads &value);
bool ApplicationThreads_EnumToString(ApplicationThreads value, string &name);

