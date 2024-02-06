#include "Neo.h"
#include "TimeManager.h"

DECLARE_MODULE(TimeManager, NeoModuleInitPri_TimeManager, NeoModulePri_Early, NeoModulePri_None);

void TimeManager::Update()
{
	auto timeNow = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = timeNow - m_lastTime;
	m_timeDelta = diff.count();
	m_lastTime = timeNow;

	LOG(Time, STR("DeltaTime {}ms {}fps", (int)(m_timeDelta * 1000), 1.0f / m_timeDelta));
}