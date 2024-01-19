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

class GraphicsThread : public Module<GraphicsThread>, Thread
{
public:
	enum Phase
	{
		PreFrame,
		MidFrame,
		Phase_MAX
	};

	GraphicsThread();
	~GraphicsThread();

	void AddTask(Phase phase, GraphicsTask task)
	{
		m_phaseTasksLock.Lock();
		m_phaseTasks[phase].emplace_back(task);
		m_phaseTasksLock.Release();
	}
	
	virtual int Go() override;

protected:
	Mutex m_phaseTasksLock;
	vector<GraphicsTask> m_phaseTasks[Phase_MAX];
	vector<GraphicsTask> m_activeTasks;
};

