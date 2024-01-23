#include "Neo.h"
#include "GILThread.h"

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


DECLARE_MODULE(GILThread, NeoModulePri_GILThread)

GILThread::GILThread() : m_nonRenderTaskThread(ThreadGUID_GILNonRender, "GILNRThread"), Thread(ThreadGUID_GILRender, "GILThread")
{
	Start();
	m_nonRenderTaskThread.Start();
	GIL::Instance().WaitTilInitialised();
}

GILThread::~GILThread()
{
}

int GILThread::Go()
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




