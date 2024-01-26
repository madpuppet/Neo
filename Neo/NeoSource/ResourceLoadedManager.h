#pragma once

#include "Neo.h"
#include "Module.h"
#include "Resource.h"
#include <functional>

// simple module that allows for thread safe callbacks when resources are finished loading
class ResourceLoadedManager : public Module<ResourceLoadedManager>
{
	fifo<std::pair<Resource*, ResourceLoadedCB>> m_resourceLoadedCallbacks;
	Mutex m_loadedLock;

public:
	void SignalResourceLoaded(Resource* resource)
	{
		// find any callbacks to this resource
		m_loadedLock.Lock();
		vector<ResourceLoadedCB> callbacks;
		for (auto cb : m_resourceLoadedCallbacks)
		{
			if (cb.first == resource)
				callbacks.push_back(cb);
		}
	}

	void AddCallback(Resource *resource, ResourceLoadedCB cb)
	{
		m_loadedLock.Lock();
		if (resource->IsLoaded())
		{
			// make callback immediately
			m_loadedLock.Release();
			cb(resource);
		}
		else
		{
			// just push onto the fifo
			m_resourceLoadedCallbacks.emplace_back( resource, cb );
			m_loadedLock.Release();
		}
	}
};

