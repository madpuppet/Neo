#include "Neo.h"
#include "RenderThread.h"
#include "ImmDynamicRenderer.h"
#include "ShaderManager.h"

DECLARE_MODULE(RenderThread, NeoModuleInitPri_RenderThread, NeoModulePri_None, NeoModulePri_None);

/*

========== RENDER THREAD ================== =
... Wait SignalUpdateDone
SignalDrawStarted ==> UPDATE THREAD
GRAPHICS = > StartFrame
Do draws - add tasks to graphics thread
GRAPHICS = > EndFrame
... WaitFrameEnded

*/

RenderThread::RenderThread() : m_gilTaskThread(ThreadGUID_GILTasks, "GILThread"), Thread(ThreadGUID_Render, "RenderThread")
{
	// do this on main thread
	auto& gil = GIL::Instance();
	gil.StartupMainThread();
	Start();
	while (!m_gilInitialized);
}

void RenderThread::AddPreDrawTask(const GenericCallback& task)
{
	ScopedMutexLock lock(m_preDrawTaskLock);
	m_preDrawTasks.push_back(task);
}

RenderThread::~RenderThread()
{
	StopAndWait();
}

int RenderThread::Go()
{
	auto& gil = GIL::Instance();
	gil.Startup();
	m_gilTaskThread.Start();
	m_gilInitialized = true;

	// module startup tasks
	m_doStartupTasks.Wait();

	// need these created before any other shader resources can create
	ShaderManager::Instance().CreateInstances();

	m_preDrawTaskLock.Lock();
	vector<GenericCallback> tasks = std::move(m_preDrawTasks);
	m_preDrawTaskLock.Release();
	for (auto& task : tasks)
	{
		task();
	}
	m_startupTasksComplete.Signal();

	// main draw loop - starts after the first Update() is finished
	while (!m_terminate)
	{
		WaitUpdateDone();

		if (m_terminate)
			break;

		// call all registered draw tasks
		m_preDrawTaskLock.Lock();
		vector<GenericCallback> tasks = std::move(m_preDrawTasks);
		m_preDrawTaskLock.Release();

		// wait for draw fences to come in
		gil.FrameWait();

		// signalling draw started allows the next update frame to begin
		SignalDrawStarted();

		for (auto& task : tasks)
		{
			task();
		}

		gil.BeginFrame();

		// call all registered draw tasks
		for (auto& item : m_drawTasks)
		{
			item.task();
		}

		gil.EndFrame();
	}

	gil.Shutdown();
	LOG(Render, "render thread terminate..");
	return 0;
}

CallbackHandle RenderThread::AddDrawTask(const GenericCallback& task, int priority)
{
	ScopedMutexLock lock(m_drawTaskLock);
	auto handle = AllocUniqueCallbackHandle();
	m_drawTasks.emplace_back(handle, priority, task);
	std::sort(m_drawTasks.begin(), m_drawTasks.end(), [](const RenderTask& a, const RenderTask& b) { return a.priority < b.priority; });
	return handle;
}

