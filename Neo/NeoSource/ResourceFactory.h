#pragma once

#include <map>
#include "StringUtils.h"

template <class T>
class ResourceFactory
{
protected:
	std::map<u64,T*> m_resources;

public:
	void Dump()
	{
		for (auto& ii : m_resources)
			Log(std::format("  {} ({:x})", ii.second->GetName().CStr(), ii.first));
	}

	T* Create(const std::string &name, const std::string&source)
	{
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
		if (resource && resource->DecRef() == 0)
		{
			u32 hash = resource->GetName().Hash();
			m_resources.erase(hash);
			delete resource;
		}
	}
};
