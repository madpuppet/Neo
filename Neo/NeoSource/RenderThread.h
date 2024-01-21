#pragma once

#include "Module.h"
#include "Thread.h"

class RenderThread : public Module<RenderThread>, Thread
{
	Semaphore m_updateDone;
	Semaphore m_drawStarted;

public:
	RenderThread();
	~RenderThread();

	virtual int Go() override;

	void WaitUpdateDone() { m_updateDone.Wait(); }
	void WaitDrawStarted() { m_drawStarted.Wait(); }
	void SignalUpdateDone() { m_updateDone.Signal(); }
	void SignalDrawStarted() { m_drawStarted.Signal(); }
};
