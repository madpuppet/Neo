#include "Neo.h"
#include "RenderThread.h"

DECLARE_MODULE(RenderThread, NeoModulePri_RenderThread);

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
	Start();
	while (!m_gilInitialized);
}

RenderThread::~RenderThread()
{
}

int RenderThread::Go()
{
	auto& gil = GIL::Instance();
	gil.Startup();
	m_gilTaskThread.Start();
	m_gilInitialized = true;
	while (!m_terminate)
	{
		// call all registered draw tasks
		m_preDrawTaskLock.Lock();
		for (auto& task : m_preDrawTasks)
		{
			task();		
		}
		m_preDrawTasks.clear();
		m_preDrawTaskLock.Release();

		WaitUpdateDone();
		SignalDrawStarted();

		gil.BeginFrame();

		// call all registered draw tasks
		for (auto& item : m_drawTasks)
		{
			item.task();
		}

		gil.EndFrame();
	}

	gil.Shutdown();
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

