#pragma once
void GameInitReflection();

enum ApplicationThreads;
bool ApplicationThreads_StringToEnum(string name, ApplicationThreads &value);
string ApplicationThreads_EnumToString(ApplicationThreads value);

