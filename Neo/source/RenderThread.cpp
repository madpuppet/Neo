#include "Neo.h"
#include "RenderThread.h"
#include "ImmDynamicRenderer.h"
#include "ShaderManager.h"

//DECLARE_MODULE(RenderThread, NeoModuleInitPri_RenderThread, NeoModulePri_None);

__RegisterModuleRenderThread::__RegisterModuleRenderThread()
{
	printf("Module Linked\n");
	NeoRegisterModule([]() -> ModuleBase* { return new RenderThread(); }, []() { delete& RenderThread::Instance(); }, "RenderThread", NeoModuleInitPri_RenderThread, NeoModulePri_None);
} 
__RegisterModuleRenderThread __registerModuleRenderThread;

/*

========== RENDER THREAD ================== =
... Wait SignalUpdateDone
SignalDrawStarted ==> UPDATE THREAD
GRAPHICS = > StartFrame
Do draws - add tasks to graphics thread
GRAPHICS = > EndFrame
... WaitFrameEnded

*/

RenderThread::RenderThread() : m_gilTaskThread(ThreadGUID_GILTasks, "GILThread"), Thread(ThreadGUID_Render, "Render")
{
	// do this on main thread
	auto& gil = GIL::Instance();
	gil.StartupMainThread();
	Start();
	while (!m_gilInitialized);
}

RenderThread::~RenderThread()
{
	StopAndWait();
}

int RenderThread::Go()
{
	auto& gil = GIL::Instance();
	gil.Initialize();
	m_gilTaskThread.Start();
	m_gilInitialized = true;

	// module startup tasks
	m_doStartupTasks.Wait();

	// need these created before any other shader resources can create
	ShaderManager::Instance().CreatePlatformData();
	AssetManager::Instance().StartWork();

	m_preDrawTasks.ExecuteAndClear();
	m_startupTasksComplete.Signal();

	// main draw loop - starts after the first Update() is finished
	while (!m_terminate)
	{
		WaitUpdateDone();

		if (m_terminate)
			break;

		// clear out any queued pre draw tasks before we wait
		m_preDrawTasks.ExecuteAndClear();

		// wait for draw fences to come in
		gil.FrameWait();

		PROFILE_FRAME_SYNC();

		// signalling draw started allows the next update frame to begin
		SignalDrawStarted();

		gil.BeginFrame();

		m_beginFrameTasks.Execute();
		m_activeRenderScene->Execute();
		m_endFrameTasks.Execute();

		gil.EndFrame();
	}

	gil.Shutdown();
	LOG(Render, "render thread terminate..");
	return 0;
}

