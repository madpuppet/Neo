#pragma once

// priority of neo modules - lowest number gets initialised first
// game should manage its own priorities and should be after the NeoModulePri_MAX
enum NeoModulePri
{
	NeoModulePri_FileManager,
	NeoModulePri_AssetManager,
	NeoModulePri_ResourceLoadedManager,
	NeoModulePri_TextureFactory,
	NeoModulePri_ShaderFactory,
	NeoModulePri_MaterialFactory,
	NeoModulePri_ModelFactory,
	NeoModulePri_GIL,
	NeoModulePri_GILThread,

	NeoModulePri_MAX
};

template<class T>
class Module
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



#define DECLARE_MODULE(x, pri) \
class __RegisterModule##x  \
{  \
public:  \
	__RegisterModule##x()  \
	{  \
		RegisterModule([]() { new x(); }, []() { delete& x::Instance(); }, #x, pri);  \
	}  \
} __registerModule##x;

void RegisterModule(std::function<void()> newFunc, std::function<void()> deleteFunc, string name, int priority);
void StartupModules();
void ShutdownModules();
