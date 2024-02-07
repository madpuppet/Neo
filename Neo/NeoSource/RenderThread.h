#pragma once

#include "Module.h"
#include "Thread.h"

class RenderThread : public Module<RenderThread>, public Thread
{
	// semaphores to sync with update thread
	Semaphore m_updateDone;
	Semaphore m_drawStarted;

	// create a thread for running GIL tasks async
	// not all things can be run on GIL thread - ie. command queue stuff must be run on render thread
	WorkerThread m_gilTaskThread;
//	fifo<GenericCallback> m_preDrawTasks;

	vector<GenericCallback> m_preDrawTasks;
	Mutex m_preDrawTaskLock;

	// list of tasks to run during the render draw - general these add items to the render command queue
	struct RenderTask
	{
		CallbackHandle handle;
		int priority;
		GenericCallback task;
	};
	Mutex m_drawTaskLock;
	vector<RenderTask> m_drawTasks;

	// set when GIL is initialised
	volatile bool m_gilInitialized = false;

public:
	RenderThread();
	~RenderThread();

	// add a recurring draw task - gets called every draw
	// if this is called during a draw, it will stall till the draw is over
	CallbackHandle AddDrawTask(const GenericCallback& task, int priority);

	// add a task that needs to run on the GIL to create gil resources
	// cannot use command queue or other resources that must be done on the main thread
	void AddGILTask(const GenericCallback& task)
	{
		m_gilTaskThread.AddTask(task);
	}

	void AddPreDrawTask(const GenericCallback& task);

	virtual int Go() override;

	virtual void Terminate()
	{
		m_terminate = true;
		SignalUpdateDone();
	}

	// synchronisation functions for syncing update & draw threads
	// these called by the draw thread
	void WaitUpdateDone() { m_updateDone.Wait(); }
	void SignalDrawStarted() { m_drawStarted.Signal(); }

	// these called by the update thread
	void WaitDrawStarted() { m_drawStarted.Wait(); }
	void SignalUpdateDone() { m_updateDone.Signal(); }
};
