#pragma once

#include "Neo.h"
#include "Module.h"
#include "Resource.h"
#include "GILThread.h"
#include <functional>

// simple module that allows for thread safe callbacks when resources are finished loading
// for now we are using a bit of brute force in that we only have one directional links
// - that is, each time a resource is loaded, we need to search all dependancy lists for anything that is waiting on this
// if we get performance issues, we could tack a list onto every resource of resources that are dependant on that resource
// -- this would be more complex and eat more memory, so I'll only do that if the current method becomes a performance issue
// none of this is happening on any critical thread, so not expecting this to be a performance bottleneck

class ResourceLoadedManager : public Module<ResourceLoadedManager>
{
	struct DependancyInfo
	{
		GenericCallback task;
		vector<Resource*> dependancies;
		int completed = 0;
	};
	vector<DependancyInfo*> m_dependancyLists;

	// this mutex locks any resources so they can only complete once at a time, and can't complete if this module is currently looking at a dependancy list
	Mutex m_mutex;

public:
	void SignalResourceLoaded(Resource* resource);
	void AddDependancyList(vector<Resource*>& list, GenericCallback cb);
};

