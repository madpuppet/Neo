#pragma once

// priority of neo modules - lowest number gets initialised first
// game should manage its own priorities and should be >= NeoModuleInitPri_Application
//<REFLECT>
enum NeoModuleInitPri
{
	NeoModuleInitPri_Reflect,
	NeoModuleInitPri_GIL,
	NeoModuleInitPri_PIL,
	NeoModuleInitPri_FileManager,
	NeoModuleInitPri_AssetManager,
	NeoModuleInitPri_TimeManager,
	NeoModuleInitPri_ResourceLoadedManager,
	NeoModuleInitPri_TextureFactory,
	NeoModuleInitPri_ShaderFactory,
	NeoModuleInitPri_MaterialFactory,
	NeoModuleInitPri_StaticMeshFactory,
	NeoModuleInitPri_BitmapFontFactory,
	NeoModuleInitPri_RenderPassFactory,
	NeoModuleInitPri_RenderSceneFactory,
	NeoModuleInitPri_RenderThread,
	NeoModuleInitPri_ShaderManager,
	NeoModuleInitPri_DefDynamicRenderer,
	NeoModuleInitPri_ImmDynamicRenderer,
	NeoModuleInitPri_Profiler,
	NeoModuleInitPri_View,

	NeoModuleInitPri_Application = 1000
};

//<REFLECT>
enum NeoModulePri
{
	NeoModulePri_None,
	NeoModulePri_First = 100,
	NeoModulePri_Early = 200,
	NeoModulePri_Mid = 300,
	NeoModulePri_Late = 400,
	NeoModulePri_Last = 500
};

class ModuleBase
{
public:
	virtual void Update() {};
	virtual void Draw() {};

	virtual ~ModuleBase() {};

	virtual void Startup() {};		// called after constructor is complete
	virtual void Shutdown() {};		// called before destructor is called
};

template<class T>
class Module : public ModuleBase
{
	static T* s_instance;

public:
	Module()
	{
		Assert(!Exists(), "Multiple definitions of a single module??");
		s_instance = (T*)this;
	}

	virtual ~Module()
	{
		Assert(s_instance == (T*)this, "Multiple definitions of a single module??");
		s_instance = NULL;
	}

	static bool Exists()
	{
		return (s_instance != NULL);
	}

	static T& Instance()
	{
		Assert(Exists(), "Requested non-existing module??");
		return *s_instance;
	}

	// begin & end module update callbacks
	virtual void BeginUpdate() {}
	virtual void EndUpdate() {}
};
template<class T> T* Module<T>::s_instance = nullptr;


typedef std::function<ModuleBase*(void)> ModuleCreateFunc;
typedef std::function<void(void)> ModuleDestroyFunc;
#define DECLARE_MODULE(x, initPri, updatePri) \
class __RegisterModule##x  \
{  \
public:  \
	__RegisterModule##x()  \
	{  \
		NeoRegisterModule([]() -> ModuleBase* { return new x(); }, []() { delete& x::Instance(); }, #x, initPri, updatePri);  \
	}  \
} __registerModule##x;

void NeoRegisterModule(ModuleCreateFunc, ModuleDestroyFunc, string name, int initPri, int updatePri);
void NeoStartupModules();
void NeoShutdownModules();
void NeoUpdateModules();
