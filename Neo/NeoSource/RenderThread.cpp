#include "Neo.h"
#include "RenderThread.h"
#include "GraphicsThread.h"

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
	auto& gt = GraphicsThread::Instance();
	while (!m_terminate)
	{
		WaitUpdateDone();
		SignalDrawStarted();

		gt.AddRenderTask([]() { GIL::Instance().BeginFrame(); });

		// ... do draw functions ...

		gt.AddRenderTask([]() { GIL::Instance().EndFrame(); });
	}
	return 0;
}
