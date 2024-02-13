#pragma once
#include <chrono>
#include "Module.h"

#define NeoTimeDelta TimeManager::Instance().TimeDelta()
#define NeoFixedTimeDelta (1.0/60.0)
#define NeoTimeNow std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()

class TimeManager : public Module<TimeManager>
{
	std::chrono::steady_clock::time_point m_lastTime;
	double m_timeDelta = 1.0 / 60.0;

	void Update();

public:
	double TimeDelta() { return m_timeDelta; }
};

