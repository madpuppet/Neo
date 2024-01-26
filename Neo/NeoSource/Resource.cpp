#include "Neo.h"
#include "Resource.h"
#include "ResourceLoadedManager.h"

void Resource::OnLoadComplete()
{
	ResourceLoadedManager::Instance().SignalResourceLoaded(this);
}
