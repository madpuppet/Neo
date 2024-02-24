#pragma once

#include "Resource.h"
#include "StringUtils.h"
#include <functional>

template <class T>
class Factory : public Module<Factory<T>>
{
	hashtable<u64, T*> m_resources;

public:
	Factory();
	T* Create(const string& name)
	{
		u64 hash = StringHash64(name);
		auto it = m_resources.find(hash);
		if (it == m_resources.end())
		{
			T* resource = new T(name);
			m_resources.insert(std::pair<u64, T*>(hash, resource));

			return resource;
		}
		it->second->IncRef();
		return it->second;
	}
	void Destroy(T* resource)
	{
		if (resource && resource->DecRef() == 0)
		{
			u64 hash = StringHash64(resource->GetName());
			m_resources.erase(hash);
			delete resource;
		}
	}
};

