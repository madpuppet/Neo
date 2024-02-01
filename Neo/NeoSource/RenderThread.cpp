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
#if NEW_CODE
	// do this on main thread
	auto& gil = GIL::Instance();
	gil.StartupMainThread();
	Start();
	while (!m_gilInitialized);
#endif
}

void RenderThread::AddPreDrawTask(const GenericCallback& task)
{
	if (Thread::IsOnThread(ThreadGUID_Render))
	{
		m_preDrawTasks.push_back(task);
	}
	else
	{
		ScopedMutexLock lock(m_preDrawTaskLock);
		m_preDrawTasks.push_back(task);
	}
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
		vector<GenericCallback> tasks = std::move(m_preDrawTasks);
		m_preDrawTaskLock.Release();

		for (auto& task : tasks)
		{
			task();		
		}

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

