#pragma once

#include "Resource.h"
#include "StringUtils.h"
#include <functional>
#include <map>

template <class T>
class ResourceFactory
{
protected:
	hashtable<u64,T*> m_resources;
	Mutex m_resourceLock;

public:
	void Dump()
	{
		ScopedMutexLock lock(m_resourceLock);
		for (auto& ii : m_resources)
			Log(std::format("  {} ({:x})", ii.second->GetName().CStr(), ii.first));

	}

	T* Create(const string &name, const string &source)
	{
		ScopedMutexLock lock(m_resourceLock);
		u64 hash = StringHash64(name);
		auto it = m_resources.find(hash);
		if (it == m_resources.end())
		{
			T* resource = new T(name, source);
			m_resources.insert( std::pair<u64,T*>(hash,resource) );
			return resource;
		}
		it->second->IncRef();
		return it->second;
	}

	void Destroy(T* resource)
	{
		ScopedMutexLock lock(m_resourceLock);
		if (resource && resource->DecRef() == 0)
		{
			u32 hash = resource->GetName().Hash();
			m_resources.erase(hash);
			delete resource;
		}
	}
};

