#include "Neo.h"
#include "Module.h"

#define MAX_MODULES 256

// keep an array of module info ptrs.
// since we can't control the order of RegisterModule calls, 
//    we need to make sure there is no global initialization required for the module info array.
//    so this is a simple [] of ptrs that is allocated during each RegisterModule call
struct ModuleInfo
{
	ModuleCreateFunc newFunc;
	ModuleDestroyFunc deleteFunc;
	string name;
	int initPri;
	int updatePri;
	int drawPri;
	ModuleBase* base;
};
ModuleInfo *s_modules[MAX_MODULES];
int s_moduleCount = 0;

ModuleInfo* s_moduleUpdates[MAX_MODULES];
int s_moduleUpdateCount = 0;

void NeoRegisterModule(ModuleCreateFunc newFunc, ModuleDestroyFunc deleteFunc, string name, int initPri, int updatePri)
{
	auto module = new ModuleInfo{ newFunc, deleteFunc, name, initPri, updatePri };
	s_modules[s_moduleCount++] = module;
	if (updatePri != NeoModulePri_None)
		s_moduleUpdates[s_moduleUpdateCount++] = module;
}

void NeoStartupModules()
{
	std::sort(s_modules, s_modules + s_moduleCount, [](const ModuleInfo* a, const ModuleInfo* b) { return a->initPri < b->initPri; });
	std::sort(s_moduleUpdates, s_moduleUpdates + s_moduleUpdateCount, [](const ModuleInfo* a, const ModuleInfo* b) { return a->updatePri < b->updatePri; });

	for (int i=0; i<s_moduleCount; i++)
	{
		LOG(Module,string("Startup: ") + s_modules[i]->name);
		s_modules[i]->base = s_modules[i]->newFunc();
		s_modules[i]->base->Startup();
	}
}

void NeoShutdownModules()
{
	for (int i=s_moduleCount-1; i>=0; --i)
	{
		LOG(Module,string("Shutdown: ") + s_modules[i]->name);
		s_modules[i]->base->Shutdown();
		s_modules[i]->deleteFunc();
	}
}

void NeoUpdateModules()
{
	for (int i = 0; i < s_moduleUpdateCount; i++)
		s_moduleUpdates[i]->base->Update();
}
