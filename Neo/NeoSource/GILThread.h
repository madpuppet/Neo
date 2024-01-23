#pragma once

/*
*   Graphics Thread is a thread/module class that provides support for running tasks on a graphics thread
*   It runs the GIL tasks for each frame and interleaves any external tasks
*/

#include "Module.h"
#include "Thread.h"

#if defined(PLATFORM_Windows)
#include "GIL_Vulkan.h"
#endif

typedef std::function<void(void)> GraphicsTask;

class GILThread : public Module<GILThread>, Thread
{
	Semaphore m_frameEnded;
	WorkerThread m_nonRenderTaskThread;
	Semaphore m_renderTasksSignal;
	Mutex m_renderTasksLock;
	fifo<GraphicsTask> m_renderTasks;

public:
	GILThread();
	~GILThread();

	void AddRenderTask(GraphicsTask task)
	{
		m_renderTasksLock.Lock();
		m_renderTasks.emplace_back(std::move(task));
		m_renderTasksLock.Release();
		m_renderTasksSignal.Signal();
	}

	void AddNonRenderTask(GraphicsTask task)
	{
		m_nonRenderTaskThread.AddTask(task);
	}

	void WaitFrameEnded() { m_frameEnded.Wait(); }
	void SignalFrameEnded() { m_frameEnded.Signal(); }

	virtual int Go() override;
};

