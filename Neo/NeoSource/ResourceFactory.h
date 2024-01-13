#pragma once

#include "AssetRefresh.h"
#include <map>

namespace Mad
{
	template <class T>
	class ResourceVisitor
	{
	public:
		virtual ~ResourceVisitor() {}
		virtual void Visit(T*) {}
	};

	template <class T>
	class ResourceFactory
	{
	protected:
		std::map<u32,T*> m_resources;

	public:
		void Visit(ResourceVisitor<T> &visitor)
		{
			for (auto it = m_resources.begin(); it != m_resources.end(); ++it)
				visitor.Visit(it->second);
		}

		void Dump()
		{
			for (auto &ii : m_resources)
				Log(std::format("  {} ({:x})", ii.second->GetName().CStr(), ii.first);
		}

		T* Create(const DMString &name, const DMString &source)
		{
			u32 hash = name.Hash();
			auto it = m_resources.find(hash);
			if (it == m_resources.end())
			{
				T* resource = new T(name, source);
				m_resources.insert( std::pair<u32,T*>(hash,resource) );
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
}

