#include "Neo.h"
#include "GraphicsThread.h"

DECLARE_MODULE(GraphicsThread, NeoModulePri_GraphicsThread)

GraphicsThread::GraphicsThread() : Thread(NeoModulePri_GraphicsThread, "GraphicsThread")
{
	Start();
}

GraphicsThread::~GraphicsThread()
{
}

int GraphicsThread::Go()
{
	auto& gil = GIL::Instance();

	gil.Startup();
	while (!m_terminate)
	{
		// pre frame tasks
		m_phaseTasksLock.Lock();
		m_activeTasks = m_phaseTasks[PreFrame];
		m_phaseTasksLock.Release();
		for (auto& task : m_activeTasks)
		{
			task();
		}

		// start frame
		gil.BeginFrame();

		// mid frame tasks
		m_phaseTasksLock.Lock();
		m_activeTasks = m_phaseTasks[MidFrame];
		m_phaseTasksLock.Release();
		for (auto& task : m_activeTasks)
		{
			task();
		}

		// end the frame
		gil.EndFrame();
	}
	gil.Shutdown();
	return 0;
}





