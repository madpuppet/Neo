#include "Neo.h"
#include "RenderThread.h"
#include "GILThread.h"

/*

========== RENDER THREAD ================== =
... Wait SignalUpdateDone
SignalDrawStarted ==> UPDATE THREAD
GRAPHICS = > StartFrame
Do draws - add tasks to graphics thread
GRAPHICS = > EndFrame
... WaitFrameEnded

*/

RenderThread::RenderThread() : Thread(ThreadGUID_RenderThread, "RenderThread")
{
}

RenderThread::~RenderThread()
{
}

int RenderThread::Go()
{
	auto& gilThread = GILThread::Instance();
	while (!m_terminate)
	{
		WaitUpdateDone();
		SignalDrawStarted();

		gilThread.AddRenderTask([]() { GIL::Instance().BeginFrame(); });

		// ... do draw functions ...

		gilThread.AddRenderTask([]() { GIL::Instance().EndFrame(); });
	}
	return 0;
}
