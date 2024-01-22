#include "Neo.h"
#include "Module.h"

// keep an array of module info ptrs.
// since we can't control the order of RegisterModule calls, 
//    we need to make sure there is no global initialization required for the module info array.
//    so this is a simple [] of ptrs that is allocated during each RegisterModule call
struct ModuleInfo
{
	std::function<void()> newFunc;
	std::function<void()> deleteFunc;
	string name;
	int priority;
};
ModuleInfo *s_modules[256];
int s_moduleCount = 0;

void RegisterModule(std::function<void()> newFunc, std::function<void()> deleteFunc, string name, int priority)
{
	s_modules[s_moduleCount++] = new ModuleInfo{ newFunc, deleteFunc, name, priority };
}

void StartupModules()
{
	std::sort(s_modules, s_modules + s_moduleCount, [](const ModuleInfo* a, const ModuleInfo* b) { return a->priority < b->priority; });
	for (int i=0; i<s_moduleCount; i++)
	{
		Log(string("STARTUP MODULE: ") + s_modules[i]->name);
		s_modules[i]->newFunc();
	}
}

void ShutdownModules()
{
	for (int i=s_moduleCount-1; i>=0; --i)
	{
		Log(string("Shutdown module: ") + s_modules[i]->name);
		s_modules[i]->deleteFunc();
	}
}

