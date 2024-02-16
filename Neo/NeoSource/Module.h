#pragma once

// priority of neo modules - lowest number gets initialised first
// game should manage its own priorities and should be after the NeoModulePri_MAX
enum NeoModuleInitPri
{
	NeoModuleInitPri_FileManager,
	NeoModuleInitPri_AssetManager,
	NeoModuleInitPri_TimeManager,
	NeoModuleInitPri_ResourceLoadedManager,
	NeoModuleInitPri_TextureFactory,
	NeoModuleInitPri_ShaderFactory,
	NeoModuleInitPri_VertexShaderFactory,
	NeoModuleInitPri_PixelShaderFactory,
	NeoModuleInitPri_ComputeShaderFactory,
	NeoModuleInitPri_MaterialFactory,
	NeoModuleInitPri_StaticMeshFactory,
	NeoModuleInitPri_BitmapFontFactory,
	NeoModuleInitPri_GIL,
	NeoModuleInitPri_RenderThread,
	NeoModuleInitPri_ShaderManager,
	NeoModuleInitPri_DefDynamicRenderer,
	NeoModuleInitPri_ImmDynamicRenderer,

	NeoModuleInitPri_Application = 1000
};

enum NeoModulePri
{
	NeoModulePri_None,
	NeoModulePri_Early = 10,
	NeoModulePri_Mid = 20,
	NeoModulePri_Late = 30
};

class ModuleBase
{
public:
	virtual void Update() {};
	virtual void Draw() {};
	virtual ~ModuleBase() {};
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
		Assert(s_instance == this, "Multiple definitions of a single module??");
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
};
template<class T> T* Module<T>::s_instance = nullptr;


typedef std::function<ModuleBase*(void)> ModuleCreateFunc;
typedef std::function<void(void)> ModuleDestroyFunc;
#define DECLARE_MODULE(x, initPri, updatePri, drawPri) \
class __RegisterModule##x  \
{  \
public:  \
	__RegisterModule##x()  \
	{  \
		NeoRegisterModule([]() -> ModuleBase* { return new x(); }, []() { delete& x::Instance(); }, #x, initPri, updatePri, drawPri);  \
	}  \
} __registerModule##x;

void NeoRegisterModule(ModuleCreateFunc, ModuleDestroyFunc, string name, int initPri, int updatePri, int drawPri);
void NeoStartupModules();
void NeoShutdownModules();
void NeoUpdateModules();
void NeoDrawModules();
