#include "Neo.h"
#include "Resource.h"
#include "ResourceLoadedManager.h"

void Resource::OnLoadComplete()
{
	ResourceLoadedManager::Instance().SignalResourceLoaded(this);

	double duration = NeoTimeNow - m_creationStartTime;
	LOG(Asset, STR("Resource {} CreationTime {}ms", m_name, (int)(duration*1000)));
}
