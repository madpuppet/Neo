#pragma once

#include "Resource.h"
#include "StringUtils.h"
#include <functional>

template <class T>
class ResourceFactory
{
protected:
	hashtable<u64, T*> m_resources;

public:
	ResourceFactory();
	T* Create(const string& name, std::function<T*()> creator)
	{
		u64 hash = StringHash64(name);
		auto it = m_resources.find(hash);
		if (it == m_resources.end())
		{
			T* resource = creator();
			m_resources.insert(std::pair<u64, T*>(hash, resource));

			return resource;
		}
		it->second->IncRef();
		return it->second;
	}
	T* Create(const string& name)
	{
		return Create(name, [name]()->T* { return new T(name); });
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

