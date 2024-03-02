#pragma once

#include "Module.h"
#include "Thread.h"
#include "RenderScene.h"

// use these when deciding when to draw something
enum DrawTaskPri
{
	DrawTaskPri_StartFrame = 0,
	DrawTaskPri_BasePixel = 500,
	DrawTaskPri_EndFrame = 1000
};


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

	TaskList m_preDrawTasks;
	TaskList m_beginFrameTasks;
	TaskList m_endFrameTasks;

	// set when GIL is initialised
	volatile bool m_gilInitialized = false;

	// active render scene to drive the render passes
	RenderSceneRef m_activeRenderScene;

public:
	RenderThread();
	~RenderThread();

	// add a task that needs to run on the GIL to create gil resources
	// cannot use command queue or other resources that must be done on the main thread
	void AddGILTask(const GenericCallback& task)
	{
		m_gilTaskThread.AddTask(task);
	}

	// tasks that will execute before the main draw loop (after all previous frame work is complete)
	// note that any pre draw tasks added during module startup will execute before the first module update
	int AddPreDrawTask(const GenericCallback& task) { return m_preDrawTasks.Add(task, 0); }
	void RemovePreDrawTask(int handle) { m_preDrawTasks.Remove(handle); }

	// add a task that will run immediate at start of frame, before any render passes are set
	int AddBeginFrameTask(const GenericCallback& task) { return m_beginFrameTasks.Add(task, 0); }
	void RemoveBeginFrameTask(int handle) { m_beginFrameTasks.Remove(handle); }

	// add a task that will run at end of frame, after the last render apss
	int AddEndFrameTask(const GenericCallback& task) { return m_endFrameTasks.Add(task, 0); }
	void RemoveEndFrameTask(int handle) { m_endFrameTasks.Remove(handle); }

	// execute startup tasks - waits until they are finished before it returns
	// this should be called by main thread before the first Update() loop,  after module startups are finished
	void DoStartupTasks()
	{
		m_doStartupTasks.Signal();
		m_startupTasksComplete.Wait();
	}

	// set what render scene we should use to drive the render passes
	void SetRenderScene(RenderScene* rs) { m_activeRenderScene = rs; }

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
