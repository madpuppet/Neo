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

	// these allow synchronisation for initial startup tasks which happen after all module startups but before the first module update
	Semaphore m_doStartupTasks;
	Semaphore m_startupTasksComplete;

	Mutex m_preDrawTaskLock;
	vector<GenericCallback> m_preDrawTasks;

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

	// tasks that will execute before the main draw loop (after all previous frame work is complete)
	// note that any pre draw tasks added during module startup will execute before the first module update
	void AddPreDrawTask(const GenericCallback& task);

	// execute startup tasks - waits until they are finished before it returns
	// this should be called by main thread before the first Update() loop,  after module startups are finished
	void DoStartupTasks()
	{
		m_doStartupTasks.Signal();
		m_startupTasksComplete.Wait();
	}

	// internal GO
	virtual int Go() override;

	// terminate the render thread asap.
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
