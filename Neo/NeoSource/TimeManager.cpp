#include "Neo.h"
#include "TimeManager.h"

DECLARE_MODULE(TimeManager, NeoModuleInitPri_TimeManager, NeoModulePri_Early, NeoModulePri_None);

void TimeManager::Update()
{
	auto timeNow = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = timeNow - m_lastTime;
	m_timeDelta = diff.count();
	m_lastTime = timeNow;

	static int count = 0;
	static double avg = 0.0;
	avg = (avg + m_timeDelta) * 0.5;
	count++;
	if ((count & 0xff)==0)
		LOG(Time, STR("DeltaTime {}ms {}fps", (int)(avg * 1000), 1.0f / avg));
}
