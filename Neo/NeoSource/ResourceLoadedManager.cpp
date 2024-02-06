#include "Neo.h"
#include "ResourceLoadedManager.h"
#include "RenderThread.h"

DECLARE_MODULE(ResourceLoadedManager, NeoModuleInitPri_ResourceLoadedManager, NeoModulePri_None, NeoModulePri_None);

void ResourceLoadedManager::SignalResourceLoaded(Resource* resource)
{
	// find any callbacks to this resource
	ScopedMutexLock lock(m_mutex);

	LOG(Asset, STR("<< COMPLETED: {} [{}]",resource->GetName(), resource->GetAssetType()));

	// we need to do this here, so it is mutex locked
	resource->MarkIsLoaded();

	auto it = m_dependancyLists.begin();
	while (it != m_dependancyLists.end())
	{
		auto depInfo = *it;
		// check all the resources in this dependancy that match this resources
		for (auto dep : depInfo->dependancies)
		{
			if (dep == resource)
			{
				depInfo->completed++;
			}
		}

		LOG(Asset, STR("  deps: {}[{}] {}/{}", depInfo->resource->GetName(), depInfo->resource->GetAssetType(), depInfo->completed, depInfo->dependancies.size()));

		// if our total completed matches are resources count, then we are done
		if (depInfo->completed == depInfo->dependancies.size())
		{
			// fire off the graphics task for creating the resource platform dependant data
			RenderThread::Instance().AddPreDrawTask(depInfo->task);
			delete depInfo;
			it = m_dependancyLists.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void ResourceLoadedManager::AddDependancyList(Resource *resource, vector<Resource*>& list, GenericCallback cb)
{
	// create a new dependancy block
	ScopedMutexLock lock(m_mutex);

	// first check if we have any to wait on, before bothering with creating a dependancy block
	int completed = 0;
	for (auto res : list)
	{
		if (res->IsLoaded())
			completed++;
	}

	if (completed < list.size())
	{
		// ok, we have some resources to wait on...
		auto depInfo = new DependancyInfo;
		depInfo->resource = resource;
		depInfo->dependancies = std::move(list);
		depInfo->completed = completed;
		depInfo->task = cb;
		m_dependancyLists.push_back(depInfo);
	}
	else
	{
		// all dependants have already loaded, so we can just fire off the task now
		RenderThread::Instance().AddPreDrawTask(cb);
	}
}
