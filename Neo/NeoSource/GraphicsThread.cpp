#include "Neo.h"
#include "GraphicsThread.h"

/*

============ GRAPHICS THREAD ==============
Init vulkan  (gil.Startup)
{
	StartFrame
	==> Reset Queue

	Misc Tasks

	EndFrame
	==> Wait on GPU finished
	==> Present screen
	==> Execute Command Queue
}
*/


DECLARE_MODULE(GraphicsThread, NeoModulePri_GraphicsThread)

GraphicsThread::GraphicsThread() : m_nonRenderTaskThread(NeoModulePri_GraphicsNRThread, "GraphicsNRThread"), Thread(NeoModulePri_GraphicsThread, "GraphicsThread")
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
		m_renderTasksSignal.Wait();
		m_renderTasksLock.Lock();
		auto task = m_renderTasks.front();
		m_renderTasks.pop_front();
		m_renderTasksLock.Release();
		task();
	}
	gil.Shutdown();
	return 0;
}




