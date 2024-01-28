#pragma once

#include "Module.h"
#include "Thread.h"

class RenderThread : public Module<RenderThread>, Thread
{
	struct RenderTask
	{
		CallbackHandle handle;
		int priority;
		GenericCallback task;
	};

	Semaphore m_updateDone;
	Semaphore m_drawStarted;
	vector<RenderTask> m_drawTasks;

public:
	RenderThread();
	~RenderThread();

	CallbackHandle AddDrawTask(const GenericCallback& task, int priority)
	{
		auto handle = AllocUniqueCallbackHandle();
		m_drawTasks.emplace_back(handle, priority, task);
		std::sort(m_drawTasks.begin(), m_drawTasks.end(), [](const RenderTask& a, const RenderTask& b) { return a.priority < b.priority; });
		return handle;
	}

	virtual int Go() override;

	void WaitUpdateDone() { m_updateDone.Wait(); }
	void WaitDrawStarted() { m_drawStarted.Wait(); }
	void SignalUpdateDone() { m_updateDone.Signal(); }
	void SignalDrawStarted() { m_drawStarted.Signal(); }
};
